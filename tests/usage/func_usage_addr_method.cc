struct Foo {
  void Used();
};

void user() {
  auto x = &Foo::Used;
}


/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/usage/func_usage_addr_method.cc:1:8",
      "all_uses": ["tests/usage/func_usage_addr_method.cc:1:8", "tests/usage/func_usage_addr_method.cc:6:13"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@Used#",
      "short_name": "Used",
      "qualified_name": "Foo::Used",
      "callers": ["1@tests/usage/func_usage_addr_method.cc:6:18"],
      "all_uses": ["tests/usage/func_usage_addr_method.cc:2:8", "tests/usage/func_usage_addr_method.cc:6:18"]
    }, {
      "id": 1,
      "usr": "c:@F@user#",
      "short_name": "user",
      "qualified_name": "user",
      "definition": "tests/usage/func_usage_addr_method.cc:5:6",
      "callees": ["0@tests/usage/func_usage_addr_method.cc:6:18"],
      "all_uses": ["tests/usage/func_usage_addr_method.cc:5:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:func_usage_addr_method.cc@53@F@user#@x",
      "short_name": "x",
      "qualified_name": "x",
      "declaration": "tests/usage/func_usage_addr_method.cc:6:8",
      "all_uses": ["tests/usage/func_usage_addr_method.cc:6:8"]
    }]
}
*/