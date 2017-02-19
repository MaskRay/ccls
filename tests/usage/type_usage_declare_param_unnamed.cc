struct ForwardType;
void foo(ForwardType*) {}
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@ForwardType",
      "short_name": "ForwardType",
      "qualified_name": "ForwardType",
      "declaration": "tests/usage/type_usage_declare_param_unnamed.cc:1:8",
      "uses": ["tests/usage/type_usage_declare_param_unnamed.cc:2:22"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@ForwardType#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/usage/type_usage_declare_param_unnamed.cc:2:6"
    }],
  "variables": []
}
*/