struct Type;
static Type t;
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Type",
      "short_name": "Type",
      "qualified_name": "Type",
      "declaration": "tests/usage/type_usage_declare_static.cc:1:8",
      "uses": ["tests/usage/type_usage_declare_static.cc:2:13"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_declare_static.cc@t",
      "short_name": "t",
      "qualified_name": "t",
      "declaration": "tests/usage/type_usage_declare_static.cc:2:13",
      "initializations": ["tests/usage/type_usage_declare_static.cc:2:13"],
      "variable_type": 0
    }]
}
*/