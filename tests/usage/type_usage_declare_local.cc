struct ForwardType;
struct ImplementedType {};

void Foo() {
  ForwardType* a;
  ImplementedType b;
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@ForwardType",
      "short_name": "ForwardType",
      "qualified_name": "ForwardType",
      "declaration": "tests/usage/type_usage_declare_local.cc:1:8",
      "uses": ["tests/usage/type_usage_declare_local.cc:5:16"]
    }, {
      "id": 1,
      "usr": "c:@S@ImplementedType",
      "short_name": "ImplementedType",
      "qualified_name": "ImplementedType",
      "definition": "tests/usage/type_usage_declare_local.cc:2:8",
      "uses": ["tests/usage/type_usage_declare_local.cc:6:19"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@Foo#",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/usage/type_usage_declare_local.cc:4:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_declare_local.cc@67@F@Foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "declaration": "tests/usage/type_usage_declare_local.cc:5:16",
      "initializations": ["tests/usage/type_usage_declare_local.cc:5:16"],
      "variable_type": 0
    }, {
      "id": 1,
      "usr": "c:type_usage_declare_local.cc@86@F@Foo#@b",
      "short_name": "b",
      "qualified_name": "b",
      "declaration": "tests/usage/type_usage_declare_local.cc:6:19",
      "initializations": ["tests/usage/type_usage_declare_local.cc:6:19"],
      "variable_type": 1
    }]
}
*/