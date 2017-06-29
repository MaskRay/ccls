class Foo {
  int foo;
};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-3:2",
      "vars": [0],
      "uses": ["1:7-1:10"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@foo",
      "short_name": "foo",
      "detailed_name": "int Foo::foo",
      "definition_spelling": "2:7-2:10",
      "definition_extent": "2:3-2:10",
      "declaring_type": 0,
      "is_local": false,
      "is_macro": false,
      "uses": ["2:7-2:10"]
    }]
}
*/
