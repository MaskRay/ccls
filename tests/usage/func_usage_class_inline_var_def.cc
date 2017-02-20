static int helper() {
  return 5;
}

class Foo {
  int x = helper();
};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/usage/func_usage_class_inline_var_def.cc:5:7",
      "vars": [0],
      "all_uses": ["tests/usage/func_usage_class_inline_var_def.cc:5:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:func_usage_class_inline_var_def.cc@F@helper#",
      "short_name": "helper",
      "qualified_name": "helper",
      "definition": "tests/usage/func_usage_class_inline_var_def.cc:1:12",
      "all_uses": ["tests/usage/func_usage_class_inline_var_def.cc:1:12", "tests/usage/func_usage_class_inline_var_def.cc:6:11"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@x",
      "short_name": "x",
      "qualified_name": "Foo::x",
      "definition": "tests/usage/func_usage_class_inline_var_def.cc:6:7",
      "declaring_type": 0,
      "all_uses": ["tests/usage/func_usage_class_inline_var_def.cc:6:7"]
    }]
}
*/