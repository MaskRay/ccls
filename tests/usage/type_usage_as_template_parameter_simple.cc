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
      "definition": "1:2:7",
      "all_uses": ["1:2:7", "*1:6:8"]
    }, {
      "id": 1,
      "usr": "c:@S@S",
      "all_uses": ["1:4:8", "*1:6:19"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_as_template_parameter_simple.cc@foo",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:6:22",
      "variable_type": 0,
      "all_uses": ["1:6:22"]
    }]
}
*/