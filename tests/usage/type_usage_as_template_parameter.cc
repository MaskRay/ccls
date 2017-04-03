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
      "instantiations": [0, 1, 2],
      "uses": ["1:2:7", "*1:6:8", "*1:7:8", "*1:9:1", "*1:10:3"]
    }, {
      "id": 1,
      "usr": "c:@S@S",
      "short_name": "S",
      "qualified_name": "S",
      "definition": "1:4:8",
      "uses": ["*1:4:8", "*1:7:19", "*1:9:12", "*1:10:14"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@return_type#",
      "short_name": "return_type",
      "qualified_name": "return_type",
      "definition": "1:9:16",
      "uses": ["1:9:16"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:type_usage_as_template_parameter.cc@f0",
      "short_name": "f0",
      "qualified_name": "f0",
      "definition": "1:6:25",
      "variable_type": 0,
      "uses": ["1:6:25"]
    }, {
      "id": 1,
      "usr": "c:type_usage_as_template_parameter.cc@f1",
      "short_name": "f1",
      "qualified_name": "f1",
      "definition": "1:7:22",
      "variable_type": 0,
      "uses": ["1:7:22"]
    }, {
      "id": 2,
      "usr": "c:type_usage_as_template_parameter.cc@150@F@return_type#@local",
      "short_name": "local",
      "qualified_name": "local",
      "definition": "1:10:18",
      "variable_type": 0,
      "uses": ["1:10:18"]
    }]
}
*/
