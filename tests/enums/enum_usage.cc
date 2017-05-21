enum class Foo {
  A,
  B = 20
};

Foo x = Foo::A;

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@E@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:12-1:15",
      "definition_extent": "1:1-4:2",
      "vars": [0, 1],
      "instances": [2],
      "uses": ["1:12-1:15", "6:1-6:4", "6:9-6:12"]
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
      "is_local": false,
      "uses": ["2:3-2:4", "6:14-6:15"]
    }, {
      "id": 1,
      "usr": "c:@E@Foo@B",
      "short_name": "B",
      "detailed_name": "Foo Foo::B",
      "definition_spelling": "3:3-3:4",
      "definition_extent": "3:3-3:9",
      "variable_type": 0,
      "declaring_type": 0,
      "is_local": false,
      "uses": ["3:3-3:4"]
    }, {
      "id": 2,
      "usr": "c:@x",
      "short_name": "x",
      "detailed_name": "Foo x",
      "definition_spelling": "6:5-6:6",
      "definition_extent": "6:1-6:15",
      "variable_type": 0,
      "is_local": false,
      "uses": ["6:5-6:6"]
    }]
}
*/
