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
      "definition_spelling": "2:7-2:17",
      "definition_extent": "2:1-2:20",
      "instantiations": [0],
      "uses": ["*2:7-2:17", "*6:8-6:18"]
    }, {
      "id": 1,
      "usr": "c:@S@S",
      "uses": ["4:8-4:9", "*6:19-6:20"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:type_usage_as_template_parameter_simple.cc@foo",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition_spelling": "6:22-6:25",
      "definition_extent": "6:1-6:25",
      "variable_type": 0,
      "uses": ["6:22-6:25"]
    }]
}
*/
