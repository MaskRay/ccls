void foo();
void foo();
void foo() {}
void foo();

/*
// Note: we always use the latest seen ("most local") definition/declaration.
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
        }, {
          "spelling": "2:6-2:9",
          "extent": "2:1-2:11",
          "content": "void foo()",
          "param_spellings": []
        }, {
          "spelling": "4:6-4:9",
          "extent": "4:1-4:11",
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
