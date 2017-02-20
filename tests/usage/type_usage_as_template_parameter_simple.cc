template<typename T>
class unique_ptr;

struct S;

extern unique_ptr<S> f;

/*
// TODO: There should be an interesting usage on S as well.
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@unique_ptr",
      "all_uses": ["tests/usage/type_usage_as_template_parameter_simple.cc:2:7", "tests/usage/type_usage_as_template_parameter_simple.cc:6:8"],
      "interesting_uses": ["tests/usage/type_usage_as_template_parameter_simple.cc:6:8"]
    }, {
      "id": 1,
      "usr": "c:@S@S",
      "all_uses": ["tests/usage/type_usage_as_template_parameter_simple.cc:4:8", "tests/usage/type_usage_as_template_parameter_simple.cc:6:19"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@f",
      "short_name": "f",
      "qualified_name": "f",
      "declaration": "tests/usage/type_usage_as_template_parameter_simple.cc:6:22",
      "variable_type": 0,
      "all_uses": ["tests/usage/type_usage_as_template_parameter_simple.cc:6:22"]
    }]
}

*/