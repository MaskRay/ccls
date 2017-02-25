namespace hello {
void foo(int a, int b);
}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@N@hello@F@foo#I#I#",
      "short_name": "foo",
      "qualified_name": "hello::foo",
      "declarations": ["1:2:6"],
      "uses": ["1:2:6"]
    }],
  "variables": []
}
*/