class Foo {
  void foo();
};

void Foo::foo() {}

/*
OUTPUT:
{
  "last_modification_time": 1,
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-3:2",
      "funcs": [0],
      "uses": ["1:7-1:10", "5:6-5:9"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foo@F@foo#",
      "short_name": "foo",
      "detailed_name": "void Foo::foo()",
      "declarations": ["2:8-2:11"],
      "definition_spelling": "5:11-5:14",
      "definition_extent": "5:1-5:19",
      "declaring_type": 0
    }]
}
*/
