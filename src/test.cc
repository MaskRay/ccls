#include "test.h"

#include "indexer.h"
#include "platform.h"
#include "serializer.h"
#include "utils.h"

#include <doctest/doctest.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <loguru/loguru.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>

// The 'diff' utility is available and we can use dprintf(3).
#if _POSIX_C_SOURCE >= 200809L
#include <sys/wait.h>
#include <unistd.h>
#endif

std::string ToString(const rapidjson::Document& document) {
  rapidjson::StringBuffer buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
  writer.SetFormatOptions(
      rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);

  buffer.Clear();
  document.Accept(writer);
  return buffer.GetString();
}

void ParseTestExpectation(
    const std::string& filename,
    const std::vector<std::string>& lines_with_endings,
    TextReplacer* replacer,
    std::vector<std::string>* flags,
    std::unordered_map<std::string, std::string>* output_sections) {
  // Scan for EXTRA_FLAGS:
  {
    bool in_output = false;
    for (std::string line : lines_with_endings) {
      TrimInPlace(line);

      if (StartsWith(line, "EXTRA_FLAGS:")) {
        assert(!in_output && "multiple EXTRA_FLAGS sections");
        in_output = true;
        continue;
      }

      if (in_output && line.empty())
        break;

      if (in_output)
        flags->push_back(line);
    }
  }

  // Scan for OUTPUT:
  {
    std::string active_output_filename;
    std::string active_output_contents;

    bool in_output = false;
    for (std::string line_with_ending : lines_with_endings) {
      if (StartsWith(line_with_ending, "*/"))
        break;

      if (StartsWith(line_with_ending, "OUTPUT:")) {
        // Terminate the previous output section if we found a new one.
        if (in_output) {
          (*output_sections)[active_output_filename] = active_output_contents;
        }

        // Try to tokenize OUTPUT: based one whitespace. If there is more than
        // one token assume it is a filename.
        std::vector<std::string> tokens = SplitString(line_with_ending, " ");
        if (tokens.size() > 1) {
          active_output_filename = tokens[1];
          TrimInPlace(active_output_filename);
        } else {
          active_output_filename = filename;
        }
        active_output_contents = "";

        in_output = true;
      } else if (in_output)
        active_output_contents += line_with_ending;
    }

    if (in_output)
      (*output_sections)[active_output_filename] = active_output_contents;
  }
}

void UpdateTestExpectation(const std::string& filename,
                           const std::string& expectation,
                           const std::string& actual) {
  // Read the entire file into a string.
  std::ifstream in(filename);
  std::string str;
  str.assign(std::istreambuf_iterator<char>(in),
             std::istreambuf_iterator<char>());
  in.close();

  // Replace expectation
  auto it = str.find(expectation);
  assert(it != std::string::npos);
  str.replace(it, expectation.size(), actual);

  // Write it back out.
  WriteToFile(filename, str);
}

void DiffDocuments(std::string path,
                   std::string path_section,
                   rapidjson::Document& expected,
                   rapidjson::Document& actual) {
  std::string joined_actual_output = ToString(actual);
  std::string joined_expected_output = ToString(expected);
  std::cout << "[FAILED] " << path << " (section " << path_section << ")"
            << std::endl;

#if _POSIX_C_SOURCE >= 200809L
  char expected_file[] = "/tmp/ccls.expected.XXXXXX";
  char actual_file[] = "/tmp/ccls.actual.XXXXXX";
  int expected_fd = mkstemp(expected_file);
  int actual_fd = mkstemp(actual_file);
  dprintf(expected_fd, "%s", joined_expected_output.c_str());
  dprintf(actual_fd, "%s", joined_actual_output.c_str());
  close(expected_fd);
  close(actual_fd);
  pid_t child = fork();
  if (child == 0) {
    execlp("diff", "diff", "-U", "3", expected_file, actual_file, NULL);
    _Exit(127);
  } else {
    int status;
    waitpid(child, &status, 0);
    unlink(expected_file);
    unlink(actual_file);
    // 'diff' returns 0 or 1 if exitted normaly.
    if (WEXITSTATUS(status) <= 1)
      return;
  }
#endif
  std::vector<std::string> actual_output =
      SplitString(joined_actual_output, "\n");
  std::vector<std::string> expected_output =
      SplitString(joined_expected_output, "\n");

  std::cout << "Expected output for " << path << " (section " << path_section
            << "):" << std::endl;
  std::cout << joined_expected_output << std::endl;
  std::cout << "Actual output for " << path << " (section " << path_section
            << "):" << std::endl;
  std::cout << joined_actual_output << std::endl;
  std::cout << std::endl;
}

void VerifySerializeToFrom(IndexFile* file) {
  std::string expected = file->ToString();
  std::string serialized = Serialize(SerializeFormat::Json, *file);
  std::unique_ptr<IndexFile> result =
      Deserialize(SerializeFormat::Json, "--.cc", serialized, "<empty>",
                  std::nullopt /*expected_version*/);
  std::string actual = result->ToString();
  if (expected != actual) {
    std::cerr << "Serialization failure" << std::endl;
    assert(false);
  }
}

std::string FindExpectedOutputForFilename(
    std::string filename,
    const std::unordered_map<std::string, std::string>& expected) {
  for (const auto& entry : expected) {
    if (EndsWith(entry.first, filename))
      return entry.second;
  }

  std::cerr << "Couldn't find expected output for " << filename << std::endl;
  getchar();
  getchar();
  return "{}";
}

IndexFile* FindDbForPathEnding(
    const std::string& path,
    const std::vector<std::unique_ptr<IndexFile>>& dbs) {
  for (auto& db : dbs) {
    if (EndsWith(db->path, path))
      return db.get();
  }
  return nullptr;
}

bool RunIndexTests(const std::string& filter_path, bool enable_update) {
  SetTestOutputMode();

  // Index tests change based on the version of clang used.
  static constexpr const char* kRequiredClangVersion =
      "clang version 6.0.0 (tags/RELEASE_600/final)";
  if (GetClangVersion() != kRequiredClangVersion &&
      GetClangVersion().find("trunk") == std::string::npos) {
    std::cerr << "Index tests must be run using clang version \""
              << kRequiredClangVersion << "\" (ccls is running with \""
              << GetClangVersion() << "\")" << std::endl;
    return false;
  }

  bool success = true;
  bool update_all = false;
  // FIXME: show diagnostics in STL/headers when running tests. At the moment
  // this can be done by constructing ClangIndex index(1, 1);
  ClangIndex index;
  for (std::string path : GetFilesInFolder("index_tests", true /*recursive*/,
                                           true /*add_folder_to_path*/)) {
    bool is_fail_allowed = false;

    if (EndsWithAny(path, {".m", ".mm"})) {
      if (!RunObjectiveCIndexTests()) {
        std::cout << "Skipping \"" << path << "\" since this platform does not "
                  << "support running Objective-C tests." << std::endl;
        continue;
      }

      // objective-c tests are often not updated right away. do not bring down
      // CI if they fail.
      if (!enable_update)
        is_fail_allowed = true;
    }

    if (path.find(filter_path) == std::string::npos)
      continue;

    if (!filter_path.empty())
      std::cout << "Running " << path << std::endl;

    // Parse expected output from the test, parse it into JSON document.
    std::vector<std::string> lines_with_endings = ReadFileLines(path);
    TextReplacer text_replacer;
    std::vector<std::string> flags;
    std::unordered_map<std::string, std::string> all_expected_output;
    ParseTestExpectation(path, lines_with_endings, &text_replacer, &flags,
                         &all_expected_output);

    // Build flags.
    bool had_extra_flags = !flags.empty();
    if (!AnyStartsWith(flags, "-x"))
      flags.push_back("-xc++");
    flags.push_back("-resource-dir=" + GetDefaultResourceDirectory());
    if (had_extra_flags) {
      std::cout << "For " << path << std::endl;
      std::cout << "  flags: " << StringJoin(flags) << std::endl;
    }
    flags.push_back(path);

    // Run test.
    Config config;
    FileConsumerSharedState file_consumer_shared;
    PerformanceImportFile perf;
    auto dbs = Parse(&config, &file_consumer_shared, path, flags, {}, &perf,
                     &index, false /*dump_ast*/);

    for (const auto& entry : all_expected_output) {
      const std::string& expected_path = entry.first;
      std::string expected_output = text_replacer.Apply(entry.second);

      // FIXME: promote to utils, find and remove duplicates (ie,
      // ccls_call_tree.cc, maybe something in project.cc).
      auto basename = [](const std::string& path) -> std::string {
        size_t last_index = path.find_last_of('/');
        if (last_index == std::string::npos)
          return path;
        return path.substr(last_index + 1);
      };

      auto severity_to_string = [](const lsDiagnosticSeverity& severity) {
        switch (severity) {
          case lsDiagnosticSeverity::Error:
            return "error ";
          case lsDiagnosticSeverity::Warning:
            return "warning ";
          case lsDiagnosticSeverity::Information:
            return "information ";
          case lsDiagnosticSeverity::Hint:
            return "hint ";
        }
        assert(false && "not reached");
        return "";
      };

      // Get output from index operation.
      IndexFile* db = FindDbForPathEnding(expected_path, dbs);
      assert(db);
      if (!db->diagnostics_.empty()) {
        std::cout << "For " << path << std::endl;
        for (const lsDiagnostic& diagnostic : db->diagnostics_) {
          std::cout << "  ";
          if (diagnostic.severity)
            std::cout << severity_to_string(*diagnostic.severity);
          std::cout << basename(db->path) << ":"
                    << diagnostic.range.start.ToString() << "-"
                    << diagnostic.range.end.ToString() << ": "
                    << diagnostic.message << std::endl;
        }
      }
      std::string actual_output = "{}";
      if (db) {
        VerifySerializeToFrom(db);
        actual_output = db->ToString();
      }
      actual_output = text_replacer.Apply(actual_output);

      // Compare output via rapidjson::Document to ignore any formatting
      // differences.
      rapidjson::Document actual;
      actual.Parse(actual_output.c_str());
      rapidjson::Document expected;
      expected.Parse(expected_output.c_str());

      if (actual == expected) {
        // std::cout << "[PASSED] " << path << std::endl;
      } else {
        if (!is_fail_allowed)
          success = false;
        DiffDocuments(path, expected_path, expected, actual);
        std::cout << std::endl;
        std::cout << std::endl;
        if (enable_update) {
          std::cout
              << "[Enter to continue - type u to update test, a to update all]";
          char c = 'u';
          if (!update_all) {
            c = getchar();
            getchar();
          }

          if (c == 'a')
            update_all = true;

          if (update_all || c == 'u') {
            // Note: we use |entry.second| instead of |expected_output| because
            // |expected_output| has had text replacements applied.
            UpdateTestExpectation(path, entry.second, ToString(actual) + "\n");
          }
        }
      }
    }
  }

  return success;
}
