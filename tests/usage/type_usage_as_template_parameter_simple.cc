template<typename T>
class unique_ptr {};

struct S;

static unique_ptr<S> foo;

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@unique_ptr",
      "short_name": "unique_ptr",
      "qualified_name": "unique_ptr",
      "definition": "tests/usage/type_usage_as_template_parameter_simple.cc:2:7",
      "all_uses": ["tests/usage/type_usage_as_template_parameter_simple.cc:2:7", "tests/usage/type_usage_as_template_parameter_simple.cc:6:8"],
      "interesting_uses": ["tests/usage/type_usage_as_template_parameter_simple.cc:6:8"]
    }, {
      "id": 1,
      "usr": "c:@S@S",
      "all_uses": ["tests/usage/type_usage_as_template_parameter_simple.cc:4:8", "tests/usage/type_usage_as_template_parameter_simple.cc:6:19"],
      "interesting_uses": ["tests/usage/type_usage_as_template_parameter_simple.cc:6:19"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_as_template_parameter_simple.cc@foo",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/usage/type_usage_as_template_parameter_simple.cc:6:22",
      "variable_type": 0,
      "all_uses": ["tests/usage/type_usage_as_template_parameter_simple.cc:6:22"]
    }]
}
*/