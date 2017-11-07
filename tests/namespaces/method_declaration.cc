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
      "detailed_name": "hello::Foo",
      "definition_spelling": "2:7-2:10",
      "definition_extent": "2:1-4:2",
      "funcs": [0],
      "uses": ["2:7-2:10"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@N@hello@S@Foo@F@foo#",
      "short_name": "foo",
      "detailed_name": "void hello::Foo::foo()",
      "is_constructor": false,
      "declarations": [{
          "spelling": "3:8-3:11",
          "extent": "3:3-3:13",
          "content": "void foo()"
        }],
      "declaring_type": 0
    }]
}
*/
