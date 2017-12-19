typedef int (func)(const int *a, const int *b);
static func	g;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:typedefs.cc@T@func",
      "short_name": "func",
      "detailed_name": "func",
      "hover": "typedef int (func)(const int *a, const int *b)",
      "definition_spelling": "1:14-1:18",
      "definition_extent": "1:1-1:47",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["1:14-1:18", "2:8-2:12"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:typedefs.cc@F@g#*1I#S0_#",
      "short_name": "g",
      "detailed_name": "func g",
      "hover": "func g",
      "declarations": [{
          "spelling": "2:13-2:14",
          "extent": "2:1-2:14",
          "content": "static func g",
          "param_spellings": ["2:13-2:13", "2:13-2:13"]
        }],
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": []
}
*/