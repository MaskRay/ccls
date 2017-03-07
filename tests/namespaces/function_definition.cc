namespace hello {
void foo() {}
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@N@hello@F@foo#",
      "short_name": "foo",
      "qualified_name": "hello::foo",
      "definition": "1:2:6",
      "uses": ["1:2:6"]
    }]
}
*/
