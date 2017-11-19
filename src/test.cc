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
  return buffer.GetString();
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
    ;
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

void RunIndexTests() {
  SetTestOutputMode();

  // TODO: Assert that we need to be on clang >= 3.9.1
  bool update_all = false;
  ClangIndex index;

  for (std::string path : GetFilesInFolder("tests", true /*recursive*/,
                                           true /*add_folder_to_path*/)) {
    float memory_before = GetProcessMemoryUsedInMb();
    float memory_after = -1.;

    {
      // if (path != "tests/constructors/make_functions.cc") continue;

      Config config;
      FileConsumer::SharedState file_consumer_shared;

      // Run test.
      // std::cout << "[START] " << path << std::endl;
      PerformanceImportFile perf;
      std::vector<std::unique_ptr<IndexFile>> dbs = Parse(
          &config, &file_consumer_shared, path,
          {"-xc++", "-std=c++11",
           "-IC:/Users/jacob/Desktop/cquery/third_party/",
           "-IC:/Users/jacob/Desktop/cquery/third_party/doctest/",
           "-IC:/Users/jacob/Desktop/cquery/third_party/rapidjson/include",
           "-IC:/Users/jacob/Desktop/cquery/src",
           "-isystemC:/Program Files (x86)/Microsoft Visual "
           "Studio/2017/Community/VC/Tools/MSVC/14.10.25017/include",
           "-isystemC:/Program Files (x86)/Windows "
           "Kits/10/Include/10.0.15063.0/ucrt"},
          {}, &perf, &index, false /*dump_ast*/);

      // Parse expected output from the test, parse it into JSON document.
      std::unordered_map<std::string, std::string> all_expected_output =
          ParseTestExpectation(path);
      for (auto& entry : all_expected_output) {
        const std::string& expected_path = entry.first;
        const std::string& expected_output = entry.second;

        // Get output from index operation.
        IndexFile* db = FindDbForPathEnding(expected_path, dbs);
        std::string actual_output = "{}";
        if (db) {
          VerifySerializeToFrom(db);
          actual_output = db->ToString();
        }

        // Compare output via rapidjson::Document to ignore any formatting
        // differences.
        rapidjson::Document actual;
        actual.Parse(actual_output.c_str());
        rapidjson::Document expected;
        expected.Parse(expected_output.c_str());

        if (actual == expected) {
          std::cout << "[PASSED] " << path << std::endl;
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
            UpdateTestExpectation(path, expected_output,
                                  ToString(actual) + "\n");
          }
        }
      }

      memory_after = GetProcessMemoryUsedInMb();
    }

    float memory_cleanup = GetProcessMemoryUsedInMb();
    std::cerr << "[memory] before=" << memory_before
              << "mb, after=" << memory_after
              << "mb, cleanup=" << memory_cleanup << "mb" << std::endl;
  }

  std::cerr << "[final presleep] " << GetProcessMemoryUsedInMb() << "mb"
            << std::endl;
  // std::this_thread::sleep_for(std::chrono::seconds(10));
  // std::cerr << "[final postsleep] " << GetProcessMemoryUsedInMb() << "mb" <<
  // std::endl;
  std::cerr << std::endl;
  std::cerr << std::endl;
  std::cerr << std::endl;
  std::cerr << std::endl;
  std::cerr << std::endl;
}

// TODO: ctor/dtor, copy ctor
// TODO: Always pass IndexFile by pointer, ie, search and remove all IndexFile&
// refs.
