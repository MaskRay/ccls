namespace {
void foo();
}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:anonymous_function.cc@aN@F@foo#",
      "short_name": "foo",
      "qualified_name": "::foo",
      "declaration": "tests/namespaces/anonymous_function.cc:2:6",
      "all_uses": ["tests/namespaces/anonymous_function.cc:2:6"]
    }],
  "variables": []
}
*/