class Foo {
  void foo();
};

void Foo::foo() {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/method_definition.cc:1:7",
      "funcs": [0]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "declaration": "tests/method_definition.cc:2:8",
      "definition": "tests/method_definition.cc:5:11",
      "declaring_type": 0
    }],
  "variables": []
}
*/