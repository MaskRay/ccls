class Foo {
  static int foo;
};

int Foo::foo;

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
      "uses": ["1:7-1:10", "5:5-5:8"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@foo",
      "short_name": "foo",
      "detailed_name": "int Foo::foo",
      "declaration": "2:14-2:17",
      "definition_spelling": "5:10-5:13",
      "definition_extent": "5:1-5:13",
      "declaring_type": 0,
      "cls": 4,
      "uses": ["2:14-2:17", "5:10-5:13"]
    }]
}
*/
