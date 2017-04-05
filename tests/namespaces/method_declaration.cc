namespace hello {
class Foo {
  void foo();
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
      "definition": "2:7",
      "funcs": [0],
      "uses": ["*2:7"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@N@hello@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "hello::Foo::foo",
      "declarations": ["3:8"],
      "declaring_type": 0,
      "uses": ["3:8"]
    }]
}
*/
