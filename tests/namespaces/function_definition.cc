namespace hello {
void foo() {}
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
      "usr": "c:@N@hello@F@foo#",
      "short_name": "foo",
      "detailed_name": "void hello::foo()",
      "hover": "void hello::foo()",
      "declarations": [],
      "definition_spelling": "2:6-2:9",
      "definition_extent": "2:1-2:14",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": []
}
*/
