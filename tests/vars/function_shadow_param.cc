void foo(int p) {
  int p = 0;
}
/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#I#",
      "short_name": "foo",
      "detailed_name": "void foo(int)",
      "is_constructor": false,
      "parameter_type_descriptions": ["int"],
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-3:2"
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
      "uses": ["1:14-1:15"]
    }, {
      "id": 1,
      "usr": "c:function_shadow_param.cc@21@F@foo#I#@p",
      "short_name": "p",
      "detailed_name": "int p",
      "definition_spelling": "2:7-2:8",
      "definition_extent": "2:3-2:8",
      "is_local": true,
      "is_macro": false,
      "uses": ["2:7-2:8"]
    }]
}
*/
