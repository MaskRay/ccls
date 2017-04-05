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
      "definition": "1:8",
      "funcs": [0],
      "uses": ["*1:8", "6:13"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foo@F@Used#",
      "short_name": "Used",
      "qualified_name": "Foo::Used",
      "declarations": ["2:8"],
      "declaring_type": 0,
      "callers": ["1@6:18"],
      "uses": ["2:8", "6:18"]
    }, {
      "id": 1,
      "usr": "c:@F@user#",
      "short_name": "user",
      "qualified_name": "user",
      "definition": "5:6",
      "callees": ["0@6:18"],
      "uses": ["5:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:func_usage_addr_method.cc@53@F@user#@x",
      "short_name": "x",
      "qualified_name": "x",
      "definition": "6:8",
      "uses": ["6:8"]
    }]
}
*/
