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
      "definition": "2:7",
      "funcs": [0],
      "uses": ["*2:7", "6:6"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@N@hello@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "hello::Foo::foo",
      "declarations": ["3:8"],
      "definition": "6:11",
      "declaring_type": 0,
      "uses": ["3:8", "6:11"]
    }]
}
*/
