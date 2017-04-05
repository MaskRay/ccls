struct ForwardType;
struct ImplementedType {};

void foo(ForwardType* f, ImplementedType a) {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@ForwardType",
      "instantiations": [0],
      "uses": ["1:8-1:19", "*4:10-4:21"]
    }, {
      "id": 1,
      "usr": "c:@S@ImplementedType",
      "short_name": "ImplementedType",
      "qualified_name": "ImplementedType",
      "definition_spelling": "2:8-2:23",
      "definition_extent": "2:1-2:26",
      "instantiations": [1],
      "uses": ["*2:8-2:23", "*4:26-4:41"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@ForwardType#$@S@ImplementedType#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition_spelling": "4:6-4:9",
      "definition_extent": "4:1-4:47",
      "uses": ["4:6-4:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:type_usage_declare_param.cc@60@F@foo#*$@S@ForwardType#$@S@ImplementedType#@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition_spelling": "4:23-4:24",
      "definition_extent": "4:10-4:24",
      "variable_type": 0,
      "uses": ["4:23-4:24"]
    }, {
      "id": 1,
      "usr": "c:type_usage_declare_param.cc@76@F@foo#*$@S@ForwardType#$@S@ImplementedType#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition_spelling": "4:42-4:43",
      "definition_extent": "4:26-4:43",
      "variable_type": 1,
      "uses": ["4:42-4:43"]
    }]
}
*/
