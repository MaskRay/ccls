class Foo {
  void foo();
};

void Foo::foo() {}

/*
// TODO: We are not inserting methods into declaring TypeDef.

OUTPUT:
{
  "types": [{
      "id": 0,
      "short_name": "Foo",
      "qualified_name": "Foo",
      "declaration": null,
      "definition": "tests/method_definition.cc:1:7",
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
      "qualified_name": "Foo::foo",
      "declaration": "tests/method_definition.cc:2:8",
      "definition": "tests/method_definition.cc:5:11",
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