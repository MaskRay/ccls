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
      "definition": "1:2:7",
      "all_uses": ["1:2:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@N@hello@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "hello::Foo::foo",
      "declaration": "1:3:8",
      "all_uses": ["1:3:8"]
    }],
  "variables": []
}
*/