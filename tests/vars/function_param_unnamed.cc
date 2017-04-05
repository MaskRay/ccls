void foo(int, int) {}
/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#I#I#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-1:22",
      "uses": ["1:6-1:9"]
    }]
}
*/
