namespace hello {
void foo() {}
}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@N@hello@F@foo#",
      "short_name": "foo",
      "qualified_name": "hello::foo",
      "definition": "tests/namespaces/function_definition.cc:2:6",
      "all_uses": ["tests/namespaces/function_definition.cc:2:6"]
    }],
  "variables": []
}
*/