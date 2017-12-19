void called() {}
void caller() {
  called();
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
      "usr": "c:@F@called#",
      "short_name": "called",
      "detailed_name": "void called()",
      "hover": "void called()",
      "declarations": [],
      "definition_spelling": "1:6-1:12",
      "definition_extent": "1:1-1:17",
      "derived": [],
      "locals": [],
      "callers": ["1@3:3-3:9"],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "detailed_name": "void caller()",
      "hover": "void caller()",
      "declarations": [],
      "definition_spelling": "2:6-2:12",
      "definition_extent": "2:1-4:2",
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["0@3:3-3:9"]
    }],
  "vars": []
}
*/
