class Foo {
  void declonly();
  virtual void purevirtual() = 0;
  void def();
};

void Foo::def() {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/declaration_vs_definition/method.cc:1:7",
      "funcs": [1, 2],
      "all_uses": ["tests/declaration_vs_definition/method.cc:1:7", "tests/declaration_vs_definition/method.cc:7:6"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@declonly#",
      "short_name": "declonly",
      "qualified_name": "Foo::declonly",
      "declaration": "tests/declaration_vs_definition/method.cc:2:8",
      "all_uses": ["tests/declaration_vs_definition/method.cc:2:8"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@F@purevirtual#",
      "short_name": "purevirtual",
      "qualified_name": "Foo::purevirtual",
      "declaration": "tests/declaration_vs_definition/method.cc:3:16",
      "declaring_type": 0,
      "all_uses": ["tests/declaration_vs_definition/method.cc:3:16"]
    }, {
      "id": 2,
      "usr": "c:@S@Foo@F@def#",
      "short_name": "def",
      "qualified_name": "Foo::def",
      "declaration": "tests/declaration_vs_definition/method.cc:4:8",
      "definition": "tests/declaration_vs_definition/method.cc:7:11",
      "declaring_type": 0,
      "all_uses": ["tests/declaration_vs_definition/method.cc:4:8", "tests/declaration_vs_definition/method.cc:7:11"]
    }],
  "variables": []
}
*/