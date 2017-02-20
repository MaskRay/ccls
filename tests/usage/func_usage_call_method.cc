struct Foo {
  void Used();
};

void user() {
  Foo* f = nullptr;
  f->Used();
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/usage/func_usage_call_method.cc:1:8",
      "all_uses": ["tests/usage/func_usage_call_method.cc:1:8", "tests/usage/func_usage_call_method.cc:6:3"],
      "interesting_uses": ["tests/usage/func_usage_call_method.cc:6:3"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@Used#",
      "short_name": "Used",
      "qualified_name": "Foo::Used",
      "callers": ["1@tests/usage/func_usage_call_method.cc:7:6"],
      "all_uses": ["tests/usage/func_usage_call_method.cc:2:8", "tests/usage/func_usage_call_method.cc:7:6"]
    }, {
      "id": 1,
      "usr": "c:@F@user#",
      "short_name": "user",
      "qualified_name": "user",
      "definition": "tests/usage/func_usage_call_method.cc:5:6",
      "callees": ["0@tests/usage/func_usage_call_method.cc:7:6"],
      "all_uses": ["tests/usage/func_usage_call_method.cc:5:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:func_usage_call_method.cc@53@F@user#@f",
      "short_name": "f",
      "qualified_name": "f",
      "declaration": "tests/usage/func_usage_call_method.cc:6:8",
      "variable_type": 0,
      "all_uses": ["tests/usage/func_usage_call_method.cc:6:8", "tests/usage/func_usage_call_method.cc:7:3"]
    }]
}
*/