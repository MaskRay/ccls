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
      "definition": "2:6",
      "uses": ["2:6"]
    }]
}
*/
