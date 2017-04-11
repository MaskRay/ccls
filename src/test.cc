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

std::vector<std::string> SplitString(const std::string& str, const std::string& delimiter) {
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


void DiffDocuments(std::string path, std::string path_section, rapidjson::Document& expected, rapidjson::Document& actual) {
  std::string joined_actual_output = ToString(actual);
  std::vector<std::string> actual_output = SplitString(joined_actual_output, "\n");
  std::string joined_expected_output = ToString(expected);
  std::vector<std::string> expected_output = SplitString(joined_expected_output, "\n");

  std::cout << "[FAILED] " << path << " (section " << path_section << ")" << std::endl;
  std::cout << "Expected output for " << path << " (section " << path_section << "):" << std::endl;
  std::cout << joined_expected_output << std::endl;
  std::cout << "Actual output for " << path << " (section " << path_section << "):" << std::endl;
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

void VerifySerializeToFrom(IndexedFile* file) {
  std::string expected = file->ToString();
  std::string actual = Deserialize("--.cc", Serialize(*file)).value().ToString();
  if (expected != actual) {
    std::cerr << "Serialization failure" << std::endl;;
    assert(false);
  }
}

std::string FindExpectedOutputForFilename(std::string filename, const std::unordered_map<std::string, std::string>& expected) {
  for (const auto& entry : expected) {
    if (EndsWith(entry.first, filename))
      return entry.second;
  }

  std::cerr << "Couldn't find expected output for " << filename << std::endl;
  std::cin.get();
  std::cin.get();
  return "{}";
}

IndexedFile* FindDbForPathEnding(const std::string& path, const std::vector<std::unique_ptr<IndexedFile>>& dbs) {
  for (auto& db : dbs) {
    if (EndsWith(db->path, path))
      return db.get();
  }
  return nullptr;
}

void RunTests() {
  // TODO: Assert that we need to be on clang >= 3.9.1

  for (std::string path : GetFilesInFolder("tests", true /*recursive*/, true /*add_folder_to_path*/)) {
    //if (path != "tests/templates/specialized_func_definition.cc") continue;
    //if (path != "tests/templates/namespace_template_class_template_func_usage_folded_into_one.cc") continue;
    //if (path != "tests/multi_file/header.h") continue;
    //if (path != "tests/multi_file/simple_impl.cc") continue;
    //if (path != "tests/usage/func_called_from_template.cc") continue;
    //if (path != "tests/templates/implicit_variable_instantiation.cc") continue;
    //if (path != "tests/_empty_test.cc") continue;

    //if (path != "tests/templates/template_class_type_usage_folded_into_one.cc") continue;
    //path = "C:/Users/jacob/Desktop/superindex/indexer/" + path;

    // Parse expected output from the test, parse it into JSON document.
    std::unordered_map<std::string, std::string> all_expected_output = ParseTestExpectation(path);

    FileConsumer::SharedState file_consumer_shared;
    FileConsumer file_consumer(&file_consumer_shared);

    // Run test.
    std::cout << "[START] " << path << std::endl;
    std::vector<std::unique_ptr<IndexedFile>> dbs = Parse(&file_consumer, path, {
        "-xc++",
        "-std=c++11",
        "-IC:/Users/jacob/Desktop/superindex/indexer/third_party/",
        "-IC:/Users/jacob/Desktop/superindex/indexer/third_party/doctest/",
        "-IC:/Users/jacob/Desktop/superindex/indexer/third_party/rapidjson/include",
        "-IC:/Users/jacob/Desktop/superindex/indexer/src"
    }, false /*dump_ast*/);

    for (auto& entry : all_expected_output) {
      const std::string& expected_path = entry.first;
      const std::string& expected_output = entry.second;

      // Get output from index operation.
      IndexedFile* db = FindDbForPathEnding(expected_path, dbs);
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
      }
      else {
        DiffDocuments(path, expected_path, expected, actual);
        std::cout << std::endl;
        std::cout << std::endl;
        std::cout << "[Enter to continue]";
        std::cin.get();
        std::cin.get();
      }
    }
  }

  std::cin.get();
}

// TODO: ctor/dtor, copy ctor
// TODO: Always pass IndexedFile by pointer, ie, search and remove all IndexedFile& refs.
// TODO: Rename IndexedFile to IndexFile

