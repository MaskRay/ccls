namespace hello {
void foo() {}
}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "short_name": "foo",
      "qualified_name": "hello::foo",
      "declaration": null,
      "definition": "tests/function_definition_in_namespace.cc:2:6",
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