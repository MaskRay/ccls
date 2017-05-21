class Foo {
  static Foo* member;
};
Foo* Foo::member = nullptr;

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
      "uses": ["1:7-1:10", "2:10-2:13", "4:1-4:4", "4:6-4:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@member",
      "short_name": "member",
      "detailed_name": "Foo * Foo::member",
      "declaration": "2:15-2:21",
      "definition_spelling": "4:11-4:17",
      "definition_extent": "4:1-4:27",
      "variable_type": 0,
      "declaring_type": 0,
      "is_local": false,
      "uses": ["2:15-2:21", "4:11-4:17"]
    }]
}
*/
