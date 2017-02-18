void foo();

void foo() {}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": "tests/function_declaration_definition.cc:1:6",
      "definition": "tests/function_declaration_definition.cc:3:6",
      "declaring_type": null,
      "base": null,
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": [],
      "uses": []
    }],
  "variables": []
}
*/