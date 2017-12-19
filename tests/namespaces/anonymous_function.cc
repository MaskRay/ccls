namespace {
void foo();
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
      "usr": "c:anonymous_function.cc@aN@F@foo#",
      "short_name": "foo",
      "detailed_name": "void ::foo()",
      "hover": "void ::foo()",
      "declarations": [{
          "spelling": "2:6-2:9",
          "extent": "2:1-2:11",
          "content": "void foo()",
          "param_spellings": []
        }],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": []
}
*/
