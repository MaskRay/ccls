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
      "definition": "tests/usage/type_usage_declare_qualifiers.cc:1:8",
      "uses": ["tests/usage/type_usage_declare_qualifiers.cc:3:10", "tests/usage/type_usage_declare_qualifiers.cc:3:26", "tests/usage/type_usage_declare_qualifiers.cc:4:3", "tests/usage/type_usage_declare_qualifiers.cc:5:3", "tests/usage/type_usage_declare_qualifiers.cc:6:9", "tests/usage/type_usage_declare_qualifiers.cc:7:9"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#&$@S@Type#&1S1_#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/usage/type_usage_declare_qualifiers.cc:3:6",
      "callees": ["1@tests/usage/type_usage_declare_qualifiers.cc:4:8"]
    }, {
      "id": 1,
      "usr": "c:@S@Type@F@Type#",
      "callers": ["0@tests/usage/type_usage_declare_qualifiers.cc:4:8"],
      "uses": ["tests/usage/type_usage_declare_qualifiers.cc:4:8"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_declare_qualifiers.cc@28@F@foo#&$@S@Type#&1S1_#@a0",
      "short_name": "a0",
      "qualified_name": "a0",
      "declaration": "tests/usage/type_usage_declare_qualifiers.cc:3:16",
      "initializations": ["tests/usage/type_usage_declare_qualifiers.cc:3:16"],
      "variable_type": 0
    }, {
      "id": 1,
      "usr": "c:type_usage_declare_qualifiers.cc@38@F@foo#&$@S@Type#&1S1_#@a1",
      "short_name": "a1",
      "qualified_name": "a1",
      "declaration": "tests/usage/type_usage_declare_qualifiers.cc:3:32",
      "initializations": ["tests/usage/type_usage_declare_qualifiers.cc:3:32"],
      "variable_type": 0
    }, {
      "id": 2,
      "usr": "c:type_usage_declare_qualifiers.cc@59@F@foo#&$@S@Type#&1S1_#@a2",
      "short_name": "a2",
      "qualified_name": "a2",
      "declaration": "tests/usage/type_usage_declare_qualifiers.cc:4:8",
      "initializations": ["tests/usage/type_usage_declare_qualifiers.cc:4:8"],
      "variable_type": 0
    }, {
      "id": 3,
      "usr": "c:type_usage_declare_qualifiers.cc@71@F@foo#&$@S@Type#&1S1_#@a3",
      "short_name": "a3",
      "qualified_name": "a3",
      "declaration": "tests/usage/type_usage_declare_qualifiers.cc:5:9",
      "initializations": ["tests/usage/type_usage_declare_qualifiers.cc:5:9"],
      "variable_type": 0
    }, {
      "id": 4,
      "usr": "c:type_usage_declare_qualifiers.cc@84@F@foo#&$@S@Type#&1S1_#@a4",
      "short_name": "a4",
      "qualified_name": "a4",
      "declaration": "tests/usage/type_usage_declare_qualifiers.cc:6:15",
      "initializations": ["tests/usage/type_usage_declare_qualifiers.cc:6:15"],
      "variable_type": 0
    }, {
      "id": 5,
      "usr": "c:type_usage_declare_qualifiers.cc@103@F@foo#&$@S@Type#&1S1_#@a5",
      "short_name": "a5",
      "qualified_name": "a5",
      "declaration": "tests/usage/type_usage_declare_qualifiers.cc:7:21",
      "initializations": ["tests/usage/type_usage_declare_qualifiers.cc:7:21"],
      "variable_type": 0
    }]
}

*/