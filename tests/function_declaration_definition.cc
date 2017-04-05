void foo();

void foo() {}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declarations": ["1:6-1:9"],
      "definition_spelling": "3:6-3:9",
      "definition_extent": "3:1-3:14",
      "uses": ["1:6-1:9", "3:6-3:9"]
    }]
}
*/
