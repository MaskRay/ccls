void foo();
void foo();
void foo() {}
void foo();

/*
// Note: we always use the latest seen ("most local") definition/declaration.
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": "tests/declaration_vs_definition/func.cc:4:6",
      "definition": "tests/declaration_vs_definition/func.cc:3:6",
      "all_uses": ["tests/declaration_vs_definition/func.cc:1:6", "tests/declaration_vs_definition/func.cc:2:6", "tests/declaration_vs_definition/func.cc:3:6", "tests/declaration_vs_definition/func.cc:4:6"]
    }],
  "variables": []
}

*/