void foo() {
  int a;
  a = 1;
  {
    int a;
    a = 2;
  }
  a = 3;
}
/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/vars/function_shadow_local.cc:1:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:function_shadow_local.cc@16@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "declaration": "tests/vars/function_shadow_local.cc:2:7",
      "initializations": ["tests/vars/function_shadow_local.cc:2:7"],
      "uses": ["tests/vars/function_shadow_local.cc:3:3", "tests/vars/function_shadow_local.cc:8:3"]
    }, {
      "id": 1,
      "usr": "c:function_shadow_local.cc@43@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "declaration": "tests/vars/function_shadow_local.cc:5:9",
      "initializations": ["tests/vars/function_shadow_local.cc:5:9"],
      "uses": ["tests/vars/function_shadow_local.cc:6:5"]
    }]
}
*/