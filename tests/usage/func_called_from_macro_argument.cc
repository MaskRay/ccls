#define MACRO_CALL(e) e

bool called(bool a, bool b);

void caller() {
  MACRO_CALL(called(true, true));
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
      "usr": "c:@F@called#b#b#",
      "short_name": "called",
      "detailed_name": "bool called(bool, bool)",
      "declarations": [{
          "spelling": "3:6-3:12",
          "extent": "3:1-3:28",
          "content": "bool called(bool a, bool b)",
          "param_spellings": ["3:18-3:19", "3:26-3:27"]
        }],
      "derived": [],
      "locals": [],
      "callers": ["1@6:14-6:20"],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "detailed_name": "void caller()",
      "declarations": [],
      "definition_spelling": "5:6-5:12",
      "definition_extent": "5:1-7:2",
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["0@6:14-6:20"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:func_called_from_macro_argument.cc@8@macro@MACRO_CALL",
      "short_name": "MACRO_CALL",
      "detailed_name": "#define MACRO_CALL(e) e",
      "definition_spelling": "1:9-1:19",
      "definition_extent": "1:9-1:24",
      "is_local": false,
      "is_macro": true,
      "uses": ["1:9-1:19", "6:3-6:13"]
    }]
}
*/