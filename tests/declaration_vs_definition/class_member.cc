class Foo {
  int foo;
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
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-3:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0],
      "instances": [],
      "uses": ["1:7-1:10"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@foo",
      "short_name": "foo",
      "detailed_name": "int Foo::foo",
      "definition_spelling": "2:7-2:10",
      "definition_extent": "2:3-2:10",
      "declaring_type": 0,
      "cls": 4,
      "uses": ["2:7-2:10"]
    }]
}
*/
