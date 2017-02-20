namespace hello {
class Foo {
  void foo() {}
};
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@N@hello@S@Foo",
      "short_name": "Foo",
      "qualified_name": "hello::Foo",
      "definition": "tests/namespaces/method_inline_declaration.cc:2:7",
      "funcs": [0],
      "all_uses": ["tests/namespaces/method_inline_declaration.cc:2:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@N@hello@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "hello::Foo::foo",
      "definition": "tests/namespaces/method_inline_declaration.cc:3:8",
      "declaring_type": 0,
      "all_uses": ["tests/namespaces/method_inline_declaration.cc:3:8"]
    }],
  "variables": []
}
*/