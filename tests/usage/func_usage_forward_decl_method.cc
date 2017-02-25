struct Foo {
  void foo();
};

void usage() {
  Foo* f = nullptr;
  f->foo();
}
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "1:1:8",
      "funcs": [0],
      "uses": ["*1:1:8", "*1:6:3"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "declarations": ["1:2:8"],
      "declaring_type": 0,
      "callers": ["1@1:7:6"],
      "uses": ["1:2:8", "1:7:6"]
    }, {
      "id": 1,
      "usr": "c:@F@usage#",
      "short_name": "usage",
      "qualified_name": "usage",
      "definition": "1:5:6",
      "callees": ["0@1:7:6"],
      "uses": ["1:5:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:func_usage_forward_decl_method.cc@53@F@usage#@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition": "1:6:8",
      "variable_type": 0,
      "uses": ["1:6:8", "1:7:3"]
    }]
}
*/