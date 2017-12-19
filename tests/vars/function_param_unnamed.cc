void foo(int, int) {}
/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@foo#I#I#",
      "short_name": "foo",
      "detailed_name": "void foo(int, int)",
      "hover": "void foo(int, int)",
      "declarations": [],
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-1:22",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": []
}
*/
