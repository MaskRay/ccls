struct ForwardType;
struct ImplementedType {};

void foo(ForwardType* f, ImplementedType a) {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@ForwardType",
      "all_uses": ["1:1:8", "*1:4:10"]
    }, {
      "id": 1,
      "usr": "c:@S@ImplementedType",
      "short_name": "ImplementedType",
      "qualified_name": "ImplementedType",
      "definition": "1:2:8",
      "all_uses": ["1:2:8", "*1:4:26"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@ForwardType#$@S@ImplementedType#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:4:6",
      "all_uses": ["1:4:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_declare_param.cc@60@F@foo#*$@S@ForwardType#$@S@ImplementedType#@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition": "1:4:23",
      "variable_type": 0,
      "all_uses": ["1:4:23"]
    }, {
      "id": 1,
      "usr": "c:type_usage_declare_param.cc@76@F@foo#*$@S@ForwardType#$@S@ImplementedType#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:4:42",
      "variable_type": 1,
      "all_uses": ["1:4:42"]
    }]
}
*/