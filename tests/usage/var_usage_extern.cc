extern int a;

void foo() {
  a = 5;
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
      "definition": "tests/usage/var_usage_extern.cc:3:6",
      "all_uses": ["tests/usage/var_usage_extern.cc:3:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "declaration": "tests/usage/var_usage_extern.cc:1:12",
      "all_uses": ["tests/usage/var_usage_extern.cc:1:12", "tests/usage/var_usage_extern.cc:4:3"]
    }]
}
*/