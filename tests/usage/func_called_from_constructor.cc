void called() {}

struct Foo {
  Foo();
};

Foo::Foo() {
  called();
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "3:8-3:11",
      "definition_extent": "3:1-5:2",
      "funcs": [1],
      "uses": ["3:8-3:11", "4:3-4:6", "7:6-7:9", "7:1-7:4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@called#",
      "short_name": "called",
      "detailed_name": "void called()",
      "definition_spelling": "1:6-1:12",
      "definition_extent": "1:1-1:17",
      "callers": ["1@8:3-8:9"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@F@Foo#",
      "short_name": "Foo",
      "detailed_name": "void Foo::Foo()",
      "declarations": ["4:3-4:6"],
      "definition_spelling": "7:6-7:9",
      "definition_extent": "7:1-9:2",
      "declaring_type": 0,
      "callees": ["0@8:3-8:9"]
    }]
}
*/
