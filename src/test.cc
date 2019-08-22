// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "test.hh"

#include "filesystem.hh"
#include "indexer.hh"
#include "pipeline.hh"
#include "platform.hh"
#include "sema_manager.hh"
#include "serializer.hh"
#include "utils.hh"

#include <llvm/ADT/StringRef.h>
#include <llvm/Config/llvm-config.h>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <fstream>
#include <stdio.h>
#include <stdlib.h>

// The 'diff' utility is available and we can use dprintf(3).
#if _POSIX_C_SOURCE >= 200809L
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace llvm;

extern bool gTestOutputMode;

namespace ccls {
std::string toString(const rapidjson::Document &document) {
  rapidjson::StringBuffer buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
  writer.SetFormatOptions(
      rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);

  buffer.Clear();
  document.Accept(writer);
  return buffer.GetString();
}

struct TextReplacer {
  struct Replacement {
    std::string from;
    std::string to;
  };

  std::vector<Replacement> replacements;

  std::string apply(const std::string &content) {
    std::string result = content;

    for (const Replacement &replacement : replacements) {
      while (true) {
        size_t idx = result.find(replacement.from);
        if (idx == std::string::npos)
          break;

        result.replace(result.begin() + idx,
                       result.begin() + idx + replacement.from.size(),
                       replacement.to);
      }
    }

    return result;
  }
};

void trimInPlace(std::string &s) {
  auto f = [](char c) { return !isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), f));
  s.erase(std::find_if(s.rbegin(), s.rend(), f).base(), s.end());
}

std::vector<std::string> splitString(const std::string &str,
                                     const std::string &delimiter) {
  // http://stackoverflow.com/a/13172514
  std::vector<std::string> strings;

  std::string::size_type pos = 0;
  std::string::size_type prev = 0;
  while ((pos = str.find(delimiter, prev)) != std::string::npos) {
    strings.push_back(str.substr(prev, pos - prev));
    prev = pos + 1;
  }

  // To get the last substring (or only, if delimiter is not found)
  strings.push_back(str.substr(prev));

  return strings;
}

void parseTestExpectation(
    const std::string &filename,
    const std::vector<std::string> &lines_with_endings, TextReplacer *replacer,
    std::vector<std::string> *flags,
    std::unordered_map<std::string, std::string> *output_sections) {
  // Scan for EXTRA_FLAGS:
  {
    bool in_output = false;
    for (StringRef line : lines_with_endings) {
      line = line.trim();

      if (line.startswith("EXTRA_FLAGS:")) {
        assert(!in_output && "multiple EXTRA_FLAGS sections");
        in_output = true;
        continue;
      }

      if (in_output && line.empty())
        break;

      if (in_output)
        flags->push_back(line.str());
    }
  }

  // Scan for OUTPUT:
  {
    std::string active_output_filename;
    std::string active_output_contents;

    bool in_output = false;
    for (StringRef line_with_ending : lines_with_endings) {
      if (line_with_ending.startswith("*/"))
        break;

      if (line_with_ending.startswith("OUTPUT:")) {
        // Terminate the previous output section if we found a new one.
        if (in_output) {
          (*output_sections)[active_output_filename] = active_output_contents;
        }

        // Try to tokenize OUTPUT: based one whitespace. If there is more than
        // one token assume it is a filename.
        SmallVector<StringRef, 2> tokens;
        line_with_ending.split(tokens, ' ');
        if (tokens.size() > 1) {
          active_output_filename = tokens[1].str();
        } else {
          active_output_filename = filename;
        }
        active_output_contents = "";

        in_output = true;
      } else if (in_output) {
        active_output_contents += line_with_ending;
        active_output_contents.push_back('\n');
      }
    }

    if (in_output)
      (*output_sections)[active_output_filename] = active_output_contents;
  }
}

void updateTestExpectation(const std::string &filename,
                           const std::string &expectation,
                           const std::string &actual) {
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
  writeToFile(filename, str);
}

void diffDocuments(std::string path, std::string path_section,
                   rapidjson::Document &expected, rapidjson::Document &actual) {
  std::string joined_actual_output = toString(actual);
  std::string joined_expected_output = toString(expected);
  printf("[FAILED] %s (section %s)\n", path.c_str(), path_section.c_str());

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
      splitString(joined_actual_output, "\n");
  std::vector<std::string> expected_output =
      splitString(joined_expected_output, "\n");

  printf("Expected output for %s (section %s)\n:%s\n", path.c_str(),
         path_section.c_str(), joined_expected_output.c_str());
  printf("Actual output for %s (section %s)\n:%s\n", path.c_str(),
         path_section.c_str(), joined_actual_output.c_str());
}

void verifySerializeToFrom(IndexFile *file) {
  std::string expected = file->toString();
  std::string serialized = ccls::serialize(SerializeFormat::Json, *file);
  std::unique_ptr<IndexFile> result =
      ccls::deserialize(SerializeFormat::Json, "--.cc", serialized, "<empty>",
                        std::nullopt /*expected_version*/);
  std::string actual = result->toString();
  if (expected != actual) {
    fprintf(stderr, "Serialization failure\n");
    // assert(false);
  }
}

std::string findExpectedOutputForFilename(
    std::string filename,
    const std::unordered_map<std::string, std::string> &expected) {
  for (const auto &entry : expected) {
    if (StringRef(entry.first).endswith(filename))
      return entry.second;
  }

  fprintf(stderr, "Couldn't find expected output for %s\n", filename.c_str());
  getchar();
  getchar();
  return "{}";
}

IndexFile *
findDbForPathEnding(const std::string &path,
                    const std::vector<std::unique_ptr<IndexFile>> &dbs) {
  for (auto &db : dbs) {
    if (StringRef(db->path).endswith(path))
      return db.get();
  }
  return nullptr;
}

bool runIndexTests(const std::string &filter_path, bool enable_update) {
  gTestOutputMode = true;
  std::string version = LLVM_VERSION_STRING;

  // Index tests change based on the version of clang used.
  static const char kRequiredClangVersion[] = "6.0.0";
  if (version != kRequiredClangVersion &&
      version.find("svn") == std::string::npos) {
    fprintf(stderr,
            "Index tests must be run using clang version %s, ccls is running "
            "with %s\n",
            kRequiredClangVersion, version.c_str());
    return false;
  }

  bool success = true;
  bool update_all = false;
  // FIXME: show diagnostics in STL/headers when running tests. At the moment
  // this can be done by conRequestIdex index(1, 1);
  SemaManager completion(
      nullptr, nullptr, [&](std::string, std::vector<Diagnostic>) {},
      [](RequestId id) {});
  getFilesInFolder(
      "index_tests", true /*recursive*/, true /*add_folder_to_path*/,
      [&](const std::string &path) {
        bool is_fail_allowed = false;

        if (path.find(filter_path) == std::string::npos)
          return;

        if (!filter_path.empty())
          printf("Running %s\n", path.c_str());

        // Parse expected output from the test, parse it into JSON document.
        std::vector<std::string> lines_with_endings;
        {
          std::ifstream fin(path);
          for (std::string line; std::getline(fin, line);)
            lines_with_endings.push_back(line);
        }
        TextReplacer text_replacer;
        std::vector<std::string> flags;
        std::unordered_map<std::string, std::string> all_expected_output;
        parseTestExpectation(path, lines_with_endings, &text_replacer, &flags,
                             &all_expected_output);

        // Build flags.
        flags.push_back("-resource-dir=" + getDefaultResourceDirectory());
        flags.push_back(path);

        // Run test.
        g_config = new Config;
        VFS vfs;
        WorkingFiles wfiles;
        std::vector<const char *> cargs;
        for (auto &arg : flags)
          cargs.push_back(arg.c_str());
        bool ok;
        auto dbs = ccls::idx::index(&completion, &wfiles, &vfs, "", path, cargs,
                                    {}, true, ok);

        for (const auto &entry : all_expected_output) {
          const std::string &expected_path = entry.first;
          std::string expected_output = text_replacer.apply(entry.second);

          // Get output from index operation.
          IndexFile *db = findDbForPathEnding(expected_path, dbs);
          std::string actual_output = "{}";
          if (db) {
            verifySerializeToFrom(db);
            actual_output = db->toString();
          }
          actual_output = text_replacer.apply(actual_output);

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
            diffDocuments(path, expected_path, expected, actual);
            puts("\n");
            if (enable_update) {
              printf("[Enter to continue - type u to update test, a to update "
                     "all]");
              char c = 'u';
              if (!update_all) {
                c = getchar();
                getchar();
              }

              if (c == 'a')
                update_all = true;

              if (update_all || c == 'u') {
                // Note: we use |entry.second| instead of |expected_output|
                // because
                // |expected_output| has had text replacements applied.
                updateTestExpectation(path, entry.second,
                                      toString(actual) + "\n");
              }
            }
          }
        }
      });

  return success;
}
} // namespace ccls
