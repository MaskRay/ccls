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
      "definition": "1:2:7",
      "funcs": [0],
      "uses": ["*1:2:7", "1:6:6"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@N@hello@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "hello::Foo::foo",
      "declaration": "1:3:8",
      "definition": "1:6:11",
      "declaring_type": 0,
      "uses": ["1:3:8", "1:6:11"]
    }],
  "variables": []
}
*/