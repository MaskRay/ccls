class Foo {
  void foo() {}
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "hover": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-3:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["1:7-1:10"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@S@Foo@F@foo#",
      "short_name": "foo",
      "detailed_name": "void Foo::foo()",
      "hover": "void Foo::foo()",
      "declarations": [],
      "definition_spelling": "2:8-2:11",
      "definition_extent": "2:3-2:16",
      "declaring_type": 0,
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": []
}
*/
