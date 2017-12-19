void foo();

void usage() {
  foo();
}
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
      "base": [],
      "derived": [],
      "locals": [],
      "callers": ["1@4:3-4:6"],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@F@usage#",
      "short_name": "usage",
      "detailed_name": "void usage()",
      "hover": "void usage()",
      "declarations": [],
      "definition_spelling": "3:6-3:11",
      "definition_extent": "3:1-5:2",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["0@4:3-4:6"]
    }],
  "vars": []
}
*/
