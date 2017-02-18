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
      "short_name": "Foo",
      "qualified_name": "hello::Foo",
      "declaration": null,
      "definition": "tests/namespaces/method_definition.cc:2:7",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "uses": []
    }],
  "functions": [{
      "id": 0,
      "short_name": "foo",
      "qualified_name": "hello::Foo::foo",
      "declaration": "tests/namespaces/method_definition.cc:3:8",
      "definition": "tests/namespaces/method_definition.cc:6:11",
      "declaring_type": 0,
      "base": null,
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": [],
      "uses": []
    }],
  "variables": []
}
*/