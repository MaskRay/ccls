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
      "uses": ["1:1:8", "*1:5:3"]
    }, {
      "id": 1,
      "usr": "c:@S@ImplementedType",
      "short_name": "ImplementedType",
      "qualified_name": "ImplementedType",
      "definition": "1:2:8",
      "uses": ["1:2:8", "*1:6:3"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@Foo#",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "1:4:6",
      "uses": ["1:4:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_declare_local.cc@67@F@Foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:5:16",
      "variable_type": 0,
      "uses": ["1:5:16"]
    }, {
      "id": 1,
      "usr": "c:type_usage_declare_local.cc@86@F@Foo#@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition": "1:6:19",
      "variable_type": 1,
      "uses": ["1:6:19"]
    }]
}
*/