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
      "definition": "tests/usage/var_usage_shadowed_local.cc:1:6",
      "all_uses": ["tests/usage/var_usage_shadowed_local.cc:1:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:var_usage_shadowed_local.cc@16@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "tests/usage/var_usage_shadowed_local.cc:2:7",
      "all_uses": ["tests/usage/var_usage_shadowed_local.cc:2:7", "tests/usage/var_usage_shadowed_local.cc:3:3", "tests/usage/var_usage_shadowed_local.cc:8:3"]
    }, {
      "id": 1,
      "usr": "c:var_usage_shadowed_local.cc@43@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "tests/usage/var_usage_shadowed_local.cc:5:9",
      "all_uses": ["tests/usage/var_usage_shadowed_local.cc:5:9", "tests/usage/var_usage_shadowed_local.cc:6:5"]
    }]
}
*/