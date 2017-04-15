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
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-3:2",
      "funcs": [0],
      "uses": ["*1:7-1:10"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "void Foo::foo()",
      "definition_spelling": "2:8-2:11",
      "definition_extent": "2:3-2:16",
      "declaring_type": 0
    }]
}
*/
