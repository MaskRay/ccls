void foo(int, int) {}
/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#I#I#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:1:6",
      "all_uses": ["1:1:6"]
    }],
  "variables": []
}
*/