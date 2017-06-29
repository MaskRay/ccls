union Foo {
  int a;
  bool b;
};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@U@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-4:2",
      "vars": [0, 1],
      "uses": ["1:7-1:10"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@U@Foo@FI@a",
      "short_name": "a",
      "detailed_name": "int Foo::a",
      "definition_spelling": "2:7-2:8",
      "definition_extent": "2:3-2:8",
      "declaring_type": 0,
      "is_local": false,
      "is_macro": false,
      "uses": ["2:7-2:8"]
    }, {
      "id": 1,
      "usr": "c:@U@Foo@FI@b",
      "short_name": "b",
      "detailed_name": "bool Foo::b",
      "definition_spelling": "3:8-3:9",
      "definition_extent": "3:3-3:9",
      "declaring_type": 0,
      "is_local": false,
      "is_macro": false,
      "uses": ["3:8-3:9"]
    }]
}
*/
