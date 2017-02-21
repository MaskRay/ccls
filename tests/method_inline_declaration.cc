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
      "definition": "1:1:7",
      "funcs": [0],
      "uses": ["1:1:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "definition": "1:2:8",
      "declaring_type": 0,
      "uses": ["1:2:8"]
    }],
  "variables": []
}
*/