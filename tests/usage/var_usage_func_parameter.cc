void foo(int a) {
  a += 10;
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
      "definition": "tests/usage/var_usage_func_parameter.cc:1:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:var_usage_func_parameter.cc@9@F@foo#I#@a",
      "short_name": "a",
      "qualified_name": "a",
      "declaration": "tests/usage/var_usage_func_parameter.cc:1:14",
      "initializations": ["tests/usage/var_usage_func_parameter.cc:1:14"],
      "uses": ["tests/usage/var_usage_func_parameter.cc:2:3"]
    }]
}
*/