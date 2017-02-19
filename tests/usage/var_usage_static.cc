static int a;

void foo() {
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
      "definition": "tests/usage/var_usage_static.cc:3:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:var_usage_static.cc@a",
      "short_name": "a",
      "qualified_name": "a",
      "declaration": "tests/usage/var_usage_static.cc:1:12",
      "initializations": ["tests/usage/var_usage_static.cc:1:12"],
      "uses": ["tests/usage/var_usage_static.cc:4:3"]
    }]
}
*/