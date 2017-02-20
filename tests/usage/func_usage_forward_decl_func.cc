void foo();

void usage() {
  foo();
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
      "declaration": "tests/usage/func_usage_forward_decl_func.cc:1:6",
      "callers": ["1@tests/usage/func_usage_forward_decl_func.cc:4:3"],
      "all_uses": ["tests/usage/func_usage_forward_decl_func.cc:1:6", "tests/usage/func_usage_forward_decl_func.cc:4:3"]
    }, {
      "id": 1,
      "usr": "c:@F@usage#",
      "short_name": "usage",
      "qualified_name": "usage",
      "definition": "tests/usage/func_usage_forward_decl_func.cc:3:6",
      "callees": ["0@tests/usage/func_usage_forward_decl_func.cc:4:3"],
      "all_uses": ["tests/usage/func_usage_forward_decl_func.cc:3:6"]
    }],
  "variables": []
}
*/