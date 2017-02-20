void foo(int a) {
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
      "usr": "c:@F@foo#I#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/usage/var_usage_shadowed_parameter.cc:1:6",
      "all_uses": ["tests/usage/var_usage_shadowed_parameter.cc:1:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:var_usage_shadowed_parameter.cc@9@F@foo#I#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "tests/usage/var_usage_shadowed_parameter.cc:1:14",
      "all_uses": ["tests/usage/var_usage_shadowed_parameter.cc:1:14", "tests/usage/var_usage_shadowed_parameter.cc:2:3", "tests/usage/var_usage_shadowed_parameter.cc:7:3"]
    }, {
      "id": 1,
      "usr": "c:var_usage_shadowed_parameter.cc@38@F@foo#I#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "tests/usage/var_usage_shadowed_parameter.cc:4:9",
      "all_uses": ["tests/usage/var_usage_shadowed_parameter.cc:4:9", "tests/usage/var_usage_shadowed_parameter.cc:5:5"]
    }]
}
*/