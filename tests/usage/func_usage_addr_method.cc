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
      "detailed_name": "Foo",
      "definition_spelling": "1:8-1:11",
      "definition_extent": "1:1-3:2",
      "funcs": [0],
      "uses": ["1:8-1:11", "6:13-6:16"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foo@F@Used#",
      "short_name": "Used",
      "detailed_name": "void Foo::Used()",
      "declarations": ["2:8-2:12"],
      "declaring_type": 0,
      "callers": ["1@6:18-6:22"]
    }, {
      "id": 1,
      "usr": "c:@F@user#",
      "short_name": "user",
      "detailed_name": "void user()",
      "definition_spelling": "5:6-5:10",
      "definition_extent": "5:1-7:2",
      "callees": ["0@6:18-6:22"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:func_usage_addr_method.cc@53@F@user#@x",
      "short_name": "x",
      "detailed_name": "void (Foo::*)() x",
      "definition_spelling": "6:8-6:9",
      "definition_extent": "6:3-6:22",
      "is_local": true,
      "uses": ["6:8-6:9"]
    }]
}
*/
