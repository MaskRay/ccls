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
      "qualified_name": "Foo",
      "definition": "1:1:7",
      "vars": [0],
      "uses": ["*1:1:7", "*1:2:10", "*1:4:1", "1:4:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@member",
      "short_name": "member",
      "qualified_name": "Foo::member",
      "declaration": "1:2:15",
      "definition": "1:4:11",
      "variable_type": 0,
      "declaring_type": 0,
      "uses": ["1:2:15", "1:4:11"]
    }]
}
*/
