namespace hello {
class Foo {
  void foo();
};

void Foo::foo() {}
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@N@hello@S@Foo",
      "short_name": "Foo",
      "qualified_name": "hello::Foo",
      "definition": "tests/namespaces/method_definition.cc:2:7",
      "funcs": [0],
      "all_uses": ["tests/namespaces/method_definition.cc:2:7", "tests/namespaces/method_definition.cc:6:6"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@N@hello@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "hello::Foo::foo",
      "declaration": "tests/namespaces/method_definition.cc:3:8",
      "definition": "tests/namespaces/method_definition.cc:6:11",
      "declaring_type": 0,
      "all_uses": ["tests/namespaces/method_definition.cc:3:8", "tests/namespaces/method_definition.cc:6:11"]
    }],
  "variables": []
}
*/