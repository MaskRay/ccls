void foo(int p) {
  int p = 0;
}
/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#I#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/vars/function_shadow_param.cc:1:6",
      "all_uses": ["tests/vars/function_shadow_param.cc:1:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:function_shadow_param.cc@9@F@foo#I#@p",
      "short_name": "p",
      "qualified_name": "p",
      "definition": "tests/vars/function_shadow_param.cc:1:14",
      "all_uses": ["tests/vars/function_shadow_param.cc:1:14"]
    }, {
      "id": 1,
      "usr": "c:function_shadow_param.cc@21@F@foo#I#@p",
      "short_name": "p",
      "qualified_name": "p",
      "definition": "tests/vars/function_shadow_param.cc:2:7",
      "all_uses": ["tests/vars/function_shadow_param.cc:2:7"]
    }]
}
*/