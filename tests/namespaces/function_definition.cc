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
      "definition": "1:2:6",
      "all_uses": ["1:2:6"]
    }],
  "variables": []
}
*/