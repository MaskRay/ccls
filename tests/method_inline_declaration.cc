class Foo {
  void foo() {}
};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/method_inline_declaration.cc:1:7",
      "funcs": [0]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "definition": "tests/method_inline_declaration.cc:2:8",
      "declaring_type": 0
    }],
  "variables": []
}
*/