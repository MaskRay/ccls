void foo();

void foo() {}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "hover": "void foo()",
      "declarations": [{
          "spelling": "1:6-1:9",
          "extent": "1:1-1:11",
          "content": "void foo()",
          "param_spellings": []
        }],
      "definition_spelling": "3:6-3:9",
      "definition_extent": "3:1-3:14",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": []
}
*/
