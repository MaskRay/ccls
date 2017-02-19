void foo() {
  int x;
  x = 3;
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
      "definition": "tests/usage/var_usage_local.cc:1:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:var_usage_local.cc@16@F@foo#@x",
      "short_name": "x",
      "qualified_name": "x",
      "declaration": "tests/usage/var_usage_local.cc:2:7",
      "initializations": ["tests/usage/var_usage_local.cc:2:7"],
      "uses": ["tests/usage/var_usage_local.cc:3:3"]
    }]
}
*/