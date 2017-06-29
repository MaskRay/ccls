class Foo {
  static int member;
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
      "uses": ["1:7-1:10"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@member",
      "short_name": "member",
      "detailed_name": "int Foo::member",
      "declaration": "2:14-2:20",
      "is_local": false,
      "is_macro": false,
      "uses": ["2:14-2:20"]
    }]
}
*/
