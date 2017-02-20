template<typename T>
class unique_ptr;

struct S {};

static unique_ptr<bool> f0;
static unique_ptr<S> f1;

unique_ptr<S>* return_type() {
  unique_ptr<S>* local;
  return nullptr;
}
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@unique_ptr",
      "all_uses": ["tests/usage/type_usage_as_template_parameter.cc:2:7", "tests/usage/type_usage_as_template_parameter.cc:6:8", "tests/usage/type_usage_as_template_parameter.cc:7:8", "tests/usage/type_usage_as_template_parameter.cc:9:1", "tests/usage/type_usage_as_template_parameter.cc:10:3"],
      "interesting_uses": ["tests/usage/type_usage_as_template_parameter.cc:6:8", "tests/usage/type_usage_as_template_parameter.cc:7:8", "tests/usage/type_usage_as_template_parameter.cc:9:1", "tests/usage/type_usage_as_template_parameter.cc:10:3"]
    }, {
      "id": 1,
      "usr": "c:@S@S",
      "short_name": "S",
      "qualified_name": "S",
      "definition": "tests/usage/type_usage_as_template_parameter.cc:4:8",
      "all_uses": ["tests/usage/type_usage_as_template_parameter.cc:4:8", "tests/usage/type_usage_as_template_parameter.cc:7:19", "tests/usage/type_usage_as_template_parameter.cc:9:12", "tests/usage/type_usage_as_template_parameter.cc:10:14"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@return_type#",
      "short_name": "return_type",
      "qualified_name": "return_type",
      "definition": "tests/usage/type_usage_as_template_parameter.cc:9:16",
      "all_uses": ["tests/usage/type_usage_as_template_parameter.cc:9:16"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_as_template_parameter.cc@f0",
      "short_name": "f0",
      "qualified_name": "f0",
      "definition": "tests/usage/type_usage_as_template_parameter.cc:6:25",
      "variable_type": 0,
      "all_uses": ["tests/usage/type_usage_as_template_parameter.cc:6:25"]
    }, {
      "id": 1,
      "usr": "c:type_usage_as_template_parameter.cc@f1",
      "short_name": "f1",
      "qualified_name": "f1",
      "definition": "tests/usage/type_usage_as_template_parameter.cc:7:22",
      "variable_type": 0,
      "all_uses": ["tests/usage/type_usage_as_template_parameter.cc:7:22"]
    }, {
      "id": 2,
      "usr": "c:type_usage_as_template_parameter.cc@150@F@return_type#@local",
      "short_name": "local",
      "qualified_name": "local",
      "definition": "tests/usage/type_usage_as_template_parameter.cc:10:18",
      "variable_type": 0,
      "all_uses": ["tests/usage/type_usage_as_template_parameter.cc:10:18"]
    }]
}
*/