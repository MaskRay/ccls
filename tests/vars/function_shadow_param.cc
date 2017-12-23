void foo(int p) {
  { int p = 0; }
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
      "usr": "c:@F@foo#I#",
      "short_name": "foo",
      "detailed_name": "void foo(int)",
      "hover": "void foo(int)",
      "declarations": [],
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-3:2",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": "c:function_shadow_param.cc@9@F@foo#I#@p",
      "short_name": "p",
      "detailed_name": "int p",
      "definition_spelling": "1:14-1:15",
      "definition_extent": "1:10-1:15",
      "is_local": true,
      "is_macro": false,
      "is_global": false,
      "is_member": false,
      "uses": ["1:14-1:15"]
    }, {
      "id": 1,
      "usr": "c:function_shadow_param.cc@23@F@foo#I#@p",
      "short_name": "p",
      "detailed_name": "int p",
      "definition_spelling": "2:9-2:10",
      "definition_extent": "2:5-2:14",
      "is_local": true,
      "is_macro": false,
      "is_global": false,
      "is_member": false,
      "uses": ["2:9-2:10"]
    }]
}
*/
