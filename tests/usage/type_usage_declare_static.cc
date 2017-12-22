struct Type {};
static Type t;
/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@Type",
      "short_name": "Type",
      "detailed_name": "Type",
      "definition_spelling": "1:8-1:12",
      "definition_extent": "1:1-1:15",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["1:8-1:12", "2:8-2:12"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": "c:type_usage_declare_static.cc@t",
      "short_name": "t",
      "detailed_name": "Type t",
      "definition_spelling": "2:13-2:14",
      "definition_extent": "2:1-2:14",
      "variable_type": 0,
      "is_local": false,
      "is_macro": false,
      "uses": ["2:13-2:14"]
    }]
}
*/
