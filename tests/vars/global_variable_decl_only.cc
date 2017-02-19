extern int global;
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "uses": ["tests/vars/global_variable_decl_only.cc:1:12"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@global",
      "short_name": "global",
      "qualified_name": "global",
      "declaration": "tests/vars/global_variable_decl_only.cc:1:12"
    }]
}
*/