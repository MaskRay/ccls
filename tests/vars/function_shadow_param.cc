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
      "qualified_name": "foo",
      "definition": "1:6",
      "uses": ["1:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:function_shadow_param.cc@9@F@foo#I#@p",
      "short_name": "p",
      "qualified_name": "p",
      "definition": "1:14",
      "uses": ["1:14"]
    }, {
      "id": 1,
      "usr": "c:function_shadow_param.cc@21@F@foo#I#@p",
      "short_name": "p",
      "qualified_name": "p",
      "definition": "2:7",
      "uses": ["2:7"]
    }]
}
*/
