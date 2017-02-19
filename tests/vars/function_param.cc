void foo(int p0, int p1) {}
/*
OUTPUT:
{
  "types": [{
      "id": 0
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#I#I#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/vars/function_param.cc:1:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:function_param.cc@9@F@foo#I#I#@p0",
      "short_name": "p0",
      "qualified_name": "p0",
      "declaration": "tests/vars/function_param.cc:1:14",
      "initializations": ["tests/vars/function_param.cc:1:14"],
      "variable_type": 0
    }, {
      "id": 1,
      "usr": "c:function_param.cc@17@F@foo#I#I#@p1",
      "short_name": "p1",
      "qualified_name": "p1",
      "declaration": "tests/vars/function_param.cc:1:22",
      "initializations": ["tests/vars/function_param.cc:1:22"],
      "variable_type": 0
    }]
}
*/