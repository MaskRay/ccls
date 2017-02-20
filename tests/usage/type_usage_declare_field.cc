struct ForwardType;
struct ImplementedType {};

struct Foo {
  ForwardType* a;
  ImplementedType b;
};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@ForwardType",
      "all_uses": ["tests/usage/type_usage_declare_field.cc:1:8", "tests/usage/type_usage_declare_field.cc:5:3"],
      "interesting_uses": ["tests/usage/type_usage_declare_field.cc:5:3"]
    }, {
      "id": 1,
      "usr": "c:@S@ImplementedType",
      "short_name": "ImplementedType",
      "qualified_name": "ImplementedType",
      "definition": "tests/usage/type_usage_declare_field.cc:2:8",
      "all_uses": ["tests/usage/type_usage_declare_field.cc:2:8", "tests/usage/type_usage_declare_field.cc:6:3"],
      "interesting_uses": ["tests/usage/type_usage_declare_field.cc:6:3"]
    }, {
      "id": 2,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/usage/type_usage_declare_field.cc:4:8",
      "vars": [0, 1],
      "all_uses": ["tests/usage/type_usage_declare_field.cc:4:8"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@a",
      "short_name": "a",
      "qualified_name": "Foo::a",
      "declaration": "tests/usage/type_usage_declare_field.cc:5:16",
      "variable_type": 0,
      "declaring_type": 2,
      "all_uses": ["tests/usage/type_usage_declare_field.cc:5:16"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@b",
      "short_name": "b",
      "qualified_name": "Foo::b",
      "declaration": "tests/usage/type_usage_declare_field.cc:6:19",
      "variable_type": 1,
      "declaring_type": 2,
      "all_uses": ["tests/usage/type_usage_declare_field.cc:6:19"]
    }]
}
*/