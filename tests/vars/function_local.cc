void foo() {
  int a;
}
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "uses": ["tests/vars/function_local.cc:2:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/vars/function_local.cc:1:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:function_local.cc@16@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "declaration": "tests/vars/function_local.cc:2:7",
      "initializations": ["tests/vars/function_local.cc:2:7"],
      "variable_type": 0
    }]
}
*/