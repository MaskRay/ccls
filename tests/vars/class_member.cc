class Foo {
  Foo* member;
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
      "instances": [0],
      "uses": ["1:7-1:10", "2:3-2:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@member",
      "short_name": "member",
      "detailed_name": "Foo * Foo::member",
      "definition_spelling": "2:8-2:14",
      "definition_extent": "2:3-2:14",
      "variable_type": 0,
      "declaring_type": 0,
      "uses": ["2:8-2:14"]
    }]
}
*/
