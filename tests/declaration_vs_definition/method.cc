class Foo {
  void declonly();
  virtual void purevirtual() = 0;
  void def();
};

void Foo::def() {}

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
      "definition_extent": "1:1-5:2",
      "funcs": [0, 1, 2],
      "uses": ["1:7-1:10", "7:6-7:9"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foo@F@declonly#",
      "short_name": "declonly",
      "detailed_name": "void Foo::declonly()",
      "declarations": ["2:8-2:16"],
      "declaring_type": 0
    }, {
      "id": 1,
      "usr": "c:@S@Foo@F@purevirtual#",
      "short_name": "purevirtual",
      "detailed_name": "void Foo::purevirtual()",
      "declarations": ["3:16-3:27"],
      "declaring_type": 0
    }, {
      "id": 2,
      "usr": "c:@S@Foo@F@def#",
      "short_name": "def",
      "detailed_name": "void Foo::def()",
      "declarations": ["4:8-4:11"],
      "definition_spelling": "7:11-7:14",
      "definition_extent": "7:1-7:19",
      "declaring_type": 0
    }]
}
*/
