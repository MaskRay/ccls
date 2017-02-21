struct Type;
static Type t;
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Type",
      "uses": ["1:1:8", "*1:2:8"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:type_usage_declare_static.cc@t",
      "short_name": "t",
      "qualified_name": "t",
      "definition": "1:2:13",
      "variable_type": 0,
      "uses": ["1:2:13"]
    }]
}
*/