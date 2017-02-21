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
      "declaration": "1:2:6",
      "uses": ["1:2:6"]
    }],
  "variables": []
}
*/