template<typename T>
class unique_ptr {};

struct S;

static unique_ptr<S> foo;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@unique_ptr",
      "short_name": "unique_ptr",
      "detailed_name": "unique_ptr",
      "definition_spelling": "2:7-2:17",
      "definition_extent": "2:1-2:20",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["2:7-2:17", "6:8-6:18"]
    }, {
      "id": 1,
      "usr": "c:@S@S",
      "short_name": "",
      "detailed_name": "",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["4:8-4:9", "6:19-6:20"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": "c:type_usage_as_template_parameter_simple.cc@foo",
      "short_name": "foo",
      "detailed_name": "unique_ptr<S> foo",
      "definition_spelling": "6:22-6:25",
      "definition_extent": "6:1-6:25",
      "variable_type": 0,
      "cls": 3,
      "uses": ["6:22-6:25"]
    }]
}
*/
