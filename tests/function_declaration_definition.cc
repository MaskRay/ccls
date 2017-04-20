void foo();

void foo() {}

/*
OUTPUT:
{
  "last_modification_time": 1,
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "declarations": ["1:6-1:9"],
      "definition_spelling": "3:6-3:9",
      "definition_extent": "3:1-3:14"
    }]
}
*/
