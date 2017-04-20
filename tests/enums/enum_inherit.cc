enum Foo : int {
  A,
  B = 20
};

/*
OUTPUT:
{
  "last_modification_time": 1,
  "types": [{
      "id": 0,
      "usr": "c:@E@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-4:2",
      "vars": [0, 1],
      "uses": ["1:6-1:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@E@Foo@A",
      "short_name": "A",
      "detailed_name": "Foo Foo::A",
      "definition_spelling": "2:3-2:4",
      "definition_extent": "2:3-2:4",
      "variable_type": 0,
      "declaring_type": 0,
      "uses": ["2:3-2:4"]
    }, {
      "id": 1,
      "usr": "c:@E@Foo@B",
      "short_name": "B",
      "detailed_name": "Foo Foo::B",
      "definition_spelling": "3:3-3:4",
      "definition_extent": "3:3-3:9",
      "variable_type": 0,
      "declaring_type": 0,
      "uses": ["3:3-3:4"]
    }]
}
*/
