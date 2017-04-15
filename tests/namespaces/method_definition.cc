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
      "definition_spelling": "2:7-2:10",
      "definition_extent": "2:1-4:2",
      "funcs": [0],
      "uses": ["*2:7-2:10", "6:6-6:9"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@N@hello@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "void hello::Foo::foo()",
      "declarations": ["3:8-3:11"],
      "definition_spelling": "6:11-6:14",
      "definition_extent": "6:1-6:19",
      "declaring_type": 0
    }]
}
*/
