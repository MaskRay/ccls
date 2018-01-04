#include "test.h"

#include "indexer.h"
#include "platform.h"
#include "serializer.h"
#include "utils.h"

void Write(const std::vector<std::string>& strs) {
  for (const std::string& str : strs)
    std::cout << str << std::endl;
}

std::string ToString(const rapidjson::Document& document) {
  rapidjson::StringBuffer buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
  writer.SetFormatOptions(
      rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);

  buffer.Clear();
  document.Accept(writer);
  std::string output = buffer.GetString();
  return UpdateToRnNewlines(output);
}

void DiffDocuments(std::string path,
                   std::string path_section,
                   rapidjson::Document& expected,
                   rapidjson::Document& actual) {
  std::string joined_actual_output = ToString(actual);
  std::vector<std::string> actual_output =
      SplitString(joined_actual_output, "\n");
  std::string joined_expected_output = ToString(expected);
  std::vector<std::string> expected_output =
      SplitString(joined_expected_output, "\n");

  std::cout << "[FAILED] " << path << " (section " << path_section << ")"
            << std::endl;
  std::cout << "Expected output for " << path << " (section " << path_section
            << "):" << std::endl;
  std::cout << joined_expected_output << std::endl;
  std::cout << "Actual output for " << path << " (section " << path_section
            << "):" << std::endl;
  std::cout << joined_actual_output << std::endl;
  std::cout << std::endl;

  int max_diff = 5;

  size_t len = std::min(actual_output.size(), expected_output.size());
  for (int i = 0; i < len; ++i) {
    if (actual_output[i] != expected_output[i]) {
      if (--max_diff < 0) {
        std::cout << "(... more lines may differ ...)" << std::endl;
        break;
      }

      std::cout << "Line " << i << " differs:" << std::endl;
      std::cout << "  expected: " << expected_output[i] << std::endl;
      std::cout << "  actual:   " << actual_output[i] << std::endl;
    }
  }

  if (actual_output.size() > len) {
    std::cout << "Additional output in actual:" << std::endl;
    for (size_t i = len; i < actual_output.size(); ++i)
      std::cout << "  " << actual_output[i] << std::endl;
  }

  if (expected_output.size() > len) {
    std::cout << "Additional output in expected:" << std::endl;
    for (size_t i = len; i < expected_output.size(); ++i)
      std::cout << "  " << expected_output[i] << std::endl;
  }
}

void VerifySerializeToFrom(IndexFile* file) {
  std::string expected = file->ToString();
  std::unique_ptr<IndexFile> result =
      Deserialize("--.cc", Serialize(*file), nullopt /*expected_version*/);
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
  std::cin.get();
  std::cin.get();
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

void RunIndexTests(const std::string& filter_path) {
  SetTestOutputMode();

  // Index tests change based on the version of clang used.
  static constexpr const char* kRequiredClangVersion =
      "clang version 5.0.1 (tags/RELEASE_501/final)";
  if (GetClangVersion() != kRequiredClangVersion) {
    std::cerr << "Index tests must be run using clang version \""
              << kRequiredClangVersion << "\" (cquery is running with \""
              << GetClangVersion() << "\")" << std::endl;
    exit(1);
  }

  bool update_all = false;
  ClangIndex index;

  for (std::string path : GetFilesInFolder("index_tests", true /*recursive*/,
                                           true /*add_folder_to_path*/)) {
    if (!RunObjectiveCIndexTests() && EndsWithAny(path, {".m", ".mm"})) {
      std::cout << "Skipping \"" << path << "\" since this platform does not "
                << "support running Objective-C tests." << std::endl;
      continue;
    }

    if (path.find(filter_path) == std::string::npos)
      continue;

    if (!filter_path.empty())
      std::cout << "Running " << path << std::endl;

    // Parse expected output from the test, parse it into JSON document.
    std::vector<std::string> lines_with_endings = ReadLinesWithEnding(path);
    TextReplacer text_replacer;
    std::vector<std::string> flags;
    std::unordered_map<std::string, std::string> all_expected_output;
    ParseTestExpectation(path, lines_with_endings, &text_replacer, &flags,
                         &all_expected_output);

    // Build flags.
    bool had_extra_flags = !flags.empty();
    if (!AnyStartsWith(flags, "-x"))
      flags.push_back("-xc++");
    if (!AnyStartsWith(flags, "-std"))
      flags.push_back("-std=c++11");
    flags.push_back("-resource-dir=" + GetDefaultResourceDirectory());
    if (had_extra_flags) {
      std::cout << "For " << path << std::endl;
      std::cout << "  flags: " << StringJoin(flags) << std::endl;
    }

    // Run test.
    Config config;
    FileConsumerSharedState file_consumer_shared;
    PerformanceImportFile perf;
    std::vector<std::unique_ptr<IndexFile>> dbs =
        Parse(&config, &file_consumer_shared, path, flags, {}, &perf, &index,
              false /*dump_ast*/);

    for (const auto& entry : all_expected_output) {
      const std::string& expected_path = entry.first;
      std::string expected_output = text_replacer.Apply(entry.second);

      // FIXME: promote to utils, find and remove duplicates (ie,
      // cquery_call_tree.cc, maybe something in project.cc).
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
        DiffDocuments(path, expected_path, expected, actual);
        std::cout << std::endl;
        std::cout << std::endl;
        std::cout
            << "[Enter to continue - type u to update test, a to update all]";
        char c = 'u';
        if (!update_all) {
          c = (char)std::cin.get();
          std::cin.get();
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

// TODO: ctor/dtor, copy ctor
// TODO: Always pass IndexFile by pointer, ie, search and remove all IndexFile&
// refs.
