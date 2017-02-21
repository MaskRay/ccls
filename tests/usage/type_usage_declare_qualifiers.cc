struct Type {};

void foo(Type& a0, const Type& a1) {
  Type a2;
  Type* a3;
  const Type* a4;
  const Type const* a5;
}
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Type",
      "short_name": "Type",
      "qualified_name": "Type",
      "definition": "1:1:8",
      "uses": ["*1:1:8", "*1:3:10", "*1:3:26", "*1:4:3", "*1:5:3", "*1:6:9", "*1:7:9"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#&$@S@Type#&1S1_#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:3:6",
      "uses": ["1:3:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_declare_qualifiers.cc@28@F@foo#&$@S@Type#&1S1_#@a0",
      "short_name": "a0",
      "qualified_name": "a0",
      "definition": "1:3:16",
      "variable_type": 0,
      "uses": ["1:3:16"]
    }, {
      "id": 1,
      "usr": "c:type_usage_declare_qualifiers.cc@38@F@foo#&$@S@Type#&1S1_#@a1",
      "short_name": "a1",
      "qualified_name": "a1",
      "definition": "1:3:32",
      "variable_type": 0,
      "uses": ["1:3:32"]
    }, {
      "id": 2,
      "usr": "c:type_usage_declare_qualifiers.cc@59@F@foo#&$@S@Type#&1S1_#@a2",
      "short_name": "a2",
      "qualified_name": "a2",
      "definition": "1:4:8",
      "variable_type": 0,
      "uses": ["1:4:8"]
    }, {
      "id": 3,
      "usr": "c:type_usage_declare_qualifiers.cc@71@F@foo#&$@S@Type#&1S1_#@a3",
      "short_name": "a3",
      "qualified_name": "a3",
      "definition": "1:5:9",
      "variable_type": 0,
      "uses": ["1:5:9"]
    }, {
      "id": 4,
      "usr": "c:type_usage_declare_qualifiers.cc@84@F@foo#&$@S@Type#&1S1_#@a4",
      "short_name": "a4",
      "qualified_name": "a4",
      "definition": "1:6:15",
      "variable_type": 0,
      "uses": ["1:6:15"]
    }, {
      "id": 5,
      "usr": "c:type_usage_declare_qualifiers.cc@103@F@foo#&$@S@Type#&1S1_#@a5",
      "short_name": "a5",
      "qualified_name": "a5",
      "definition": "1:7:21",
      "variable_type": 0,
      "uses": ["1:7:21"]
    }]
}

*/