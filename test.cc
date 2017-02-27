#include "indexer.h"

template<typename T>
bool AreEqual(const std::vector<T>& a, const std::vector<T>& b) {
  if (a.size() != b.size())
    return false;

  for (int i = 0; i < a.size(); ++i) {
    if (a[i] != b[i])
      return false;
  }

  return true;
}

void Write(const std::vector<std::string>& strs) {
  for (const std::string& str : strs) {
    std::cout << str << std::endl;
  }
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

std::vector<std::string> split_string(const std::string& str, const std::string& delimiter) {
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


void DiffDocuments(rapidjson::Document& expected, rapidjson::Document& actual) {
  std::vector<std::string> actual_output;
  {
    std::string buffer = ToString(actual);
    actual_output = split_string(buffer, "\n");
  }

  std::vector<std::string> expected_output;
  {
    std::string buffer = ToString(expected);
    expected_output = split_string(buffer, "\n");
  }

  int len = std::min(actual_output.size(), expected_output.size());
  for (int i = 0; i < len; ++i) {
    if (actual_output[i] != expected_output[i]) {
      std::cout << "Line " << i << " differs:" << std::endl;
      std::cout << "  expected: " << expected_output[i] << std::endl;
      std::cout << "  actual:   " << actual_output[i] << std::endl;
    }
  }

  if (actual_output.size() > len) {
    std::cout << "Additional output in actual:" << std::endl;
    for (int i = len; i < actual_output.size(); ++i)
      std::cout << "  " << actual_output[i] << std::endl;
  }

  if (expected_output.size() > len) {
    std::cout << "Additional output in expected:" << std::endl;
    for (int i = len; i < expected_output.size(); ++i)
      std::cout << "  " << expected_output[i] << std::endl;
  }
}

void WriteToFile(const std::string& filename, const std::string& content) {
  std::ofstream file(filename);
  file << content;
}

int main(int argc, char** argv) {
  // TODO: Assert that we need to be on clang >= 3.9.1

  /*
  ParsingDatabase db = Parse("tests/vars/function_local.cc");
  std::cout << std::endl << "== Database ==" << std::endl;
  std::cout << db.ToString();
  std::cin.get();
  return 0;
  */

  for (std::string path : GetFilesInFolder("tests")) {
    //if (path != "tests/usage/type_usage_declare_field.cc") continue;
    //if (path != "tests/enums/enum_class_decl.cc") continue;
    //if (path != "tests/constructors/constructor.cc") continue;
    //if (path == "tests/constructors/destructor.cc") continue;
    //if (path == "tests/usage/func_usage_call_method.cc") continue;
    //if (path != "tests/usage/type_usage_as_template_parameter.cc") continue;
    //if (path != "tests/usage/type_usage_as_template_parameter_complex.cc") continue;
    //if (path != "tests/usage/type_usage_as_template_parameter_simple.cc") continue;
    //if (path != "tests/usage/type_usage_typedef_and_using.cc") continue;
    //if (path != "tests/usage/type_usage_declare_local.cc") continue;
    //if (path == "tests/usage/type_usage_typedef_and_using_template.cc") continue;
    //if (path != "tests/usage/func_usage_addr_method.cc") continue;
    //if (path != "tests/usage/type_usage_typedef_and_using.cc") continue;
    //if (path != "tests/usage/usage_inside_of_call.cc") continue;
    //if (path != "tests/foobar.cc") continue;
    //if (path != "tests/types/anonymous_struct.cc") continue;

    // Parse expected output from the test, parse it into JSON document.
    std::string expected_output;
    ParseTestExpectation(path, &expected_output);
    rapidjson::Document expected;
    expected.Parse(expected_output.c_str());

    // Run test.
    std::cout << "[START] " << path << std::endl;
    IdCache id_cache;
    IndexedFile db = Parse(&id_cache, path, {}, true /*dump_ast*/);
    std::string actual_output = db.ToString();

    //WriteToFile("output.json", actual_output);
    //break;

    rapidjson::Document actual;
    actual.Parse(actual_output.c_str());

    if (actual == expected) {
      std::cout << "[PASSED] " << path << std::endl;
    }
    else {
      std::cout << "[FAILED] " << path << std::endl;
      std::cout << "Expected output for " << path << ":" << std::endl;
      std::cout << expected_output;
      std::cout << "Actual output for " << path << ":" << std::endl;
      std::cout << actual_output;
      std::cout << std::endl;
      std::cout << std::endl;
      DiffDocuments(expected, actual);
      break;
    }
  }

  std::cin.get();
  return 0;
}

// TODO: ctor/dtor, copy ctor
