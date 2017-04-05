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
      "definition": "2:7",
      "instantiations": [0],
      "uses": ["*2:7", "*6:8"]
    }, {
      "id": 1,
      "usr": "c:@S@S",
      "uses": ["4:8", "*6:19"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:type_usage_as_template_parameter_simple.cc@foo",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "6:22",
      "variable_type": 0,
      "uses": ["6:22"]
    }]
}
*/
