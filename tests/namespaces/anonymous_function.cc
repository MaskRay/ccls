namespace {
void foo();
}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "short_name": "foo",
      "qualified_name": "::foo",
      "declaration": "tests/namespaces/anonymous_function.cc:2:6"
    }],
  "variables": []
}
*/