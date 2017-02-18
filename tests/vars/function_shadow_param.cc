void foo(int p) {
  int p = 0;
}
/*
OUTPUT:
{
  "types": [{
      "id": 0
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#I#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/vars/function_shadow_param.cc:1:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:function_shadow_param.cc@9@F@foo#I#@p",
      "short_name": "p",
      "qualified_name": "p",
      "declaration": "tests/vars/function_shadow_param.cc:1:14",
      "initializations": ["tests/vars/function_shadow_param.cc:1:14"],
      "variable_type": 0
    }, {
      "id": 1,
      "usr": "c:function_shadow_param.cc@21@F@foo#I#@p",
      "short_name": "p",
      "qualified_name": "p",
      "declaration": "tests/vars/function_shadow_param.cc:2:7",
      "initializations": ["tests/vars/function_shadow_param.cc:2:7"],
      "variable_type": 0
    }]
}
*/