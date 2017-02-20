void foo() {}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/function_definition.cc:1:6",
      "all_uses": ["tests/function_definition.cc:1:6"]
    }],
  "variables": []
}
*/