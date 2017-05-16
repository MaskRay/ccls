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

void VerifySerializeToFrom(IndexFile* file) {
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

IndexFile* FindDbForPathEnding(const std::string& path, const std::vector<std::unique_ptr<IndexFile>>& dbs) {
  for (auto& db : dbs) {
    if (EndsWith(db->path, path))
      return db.get();
  }
  return nullptr;
}

void RunTests() {
  SetTestOutputMode();

  // TODO: Assert that we need to be on clang >= 3.9.1
  bool update_all = false;

  for (std::string path : GetFilesInFolder("tests", true /*recursive*/, true /*add_folder_to_path*/)) {
    //if (path != "tests/templates/specialized_func_definition.cc") continue;
    //if (path != "tests/templates/namespace_template_class_template_func_usage_folded_into_one.cc") continue;
    //if (path != "tests/multi_file/funky_enum.cc") continue;
    //if (path != "tests/multi_file/simple_impl.cc") continue;
    //if (path != "tests/usage/func_called_implicit_ctor.cc") continue;
    //if (path != "tests/templates/implicit_variable_instantiation.cc") continue;
    //if (path != "tests/_empty_test.cc") continue;

    //if (path != "tests/templates/template_class_type_usage_folded_into_one.cc") continue;
    //path = "C:/Users/jacob/Desktop/superindex/indexer/" + path;

    // Parse expected output from the test, parse it into JSON document.
    std::unordered_map<std::string, std::string> all_expected_output = ParseTestExpectation(path);

    IndexerConfig config;
    FileConsumer::SharedState file_consumer_shared;

    // Run test.
    std::cout << "[START] " << path << std::endl;
    std::vector<std::unique_ptr<IndexFile>> dbs = Parse(
        &config, &file_consumer_shared,
        path,
        {
          "-xc++",
          "-std=c++11",
          "-IC:/Users/jacob/Desktop/superindex/indexer/third_party/",
          "-IC:/Users/jacob/Desktop/superindex/indexer/third_party/doctest/",
          "-IC:/Users/jacob/Desktop/superindex/indexer/third_party/rapidjson/include",
          "-IC:/Users/jacob/Desktop/superindex/indexer/src"
        },
        "", nullopt,
        false /*dump_ast*/);

#if false
    for (auto& db : dbs) {
      assert(db);
      if (!db) {
        std::cerr << "no db!!!" << std::endl;
        continue;
      }

      for (auto& func : db->funcs) {
        if (!func.HasInterestingState())
          continue;


        if (func.uses.size() !=
          (func.callers.size() + func.declarations.size() + (func.def.definition_spelling.has_value() ? 1 : 0))) {

          std::cout << "func.def.usr = " << func.def.usr << std::endl;
          std::cout << "func.uses.size() = " << func.uses.size() << std::endl;
          std::cout << "func.callers.size() = " << func.callers.size() << std::endl;
          std::cout << "func.declarations.size() = " << func.declarations.size() << std::endl;
          std::cout << "func.definition_spelling.has_value() = " << func.def.definition_spelling.has_value() << std::endl;

          std::cerr << "err" << std::endl;

        }
      }
    }
#endif

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
      }
      else {
        DiffDocuments(path, expected_path, expected, actual);
        std::cout << std::endl;
        std::cout << std::endl;
        std::cout << "[Enter to continue - type u to update test, a to update all]";
        char c = 'u';
        if (!update_all) {
          c = std::cin.get();
          std::cin.get();
        }

        if (c == 'a')
          update_all = true;

        if (update_all || c == 'u') {
          UpdateTestExpectation(path, expected_output, ToString(actual) + "\n");
        }
        
      }
    }
  }

  std::cin.get();
}

// TODO: ctor/dtor, copy ctor
// TODO: Always pass IndexFile by pointer, ie, search and remove all IndexFile& refs.

