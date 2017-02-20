void foo();

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
      "declaration": "tests/function_declaration_definition.cc:1:6",
      "definition": "tests/function_declaration_definition.cc:3:6",
      "all_uses": ["tests/function_declaration_definition.cc:1:6", "tests/function_declaration_definition.cc:3:6"]
    }],
  "variables": []
}
*/