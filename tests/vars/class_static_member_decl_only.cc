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
      "qualified_name": "Foo",
      "definition": "*1:1:7",
      "all_uses": ["*1:1:7"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@member",
      "short_name": "member",
      "qualified_name": "Foo::member",
      "declaration": "*1:2:14",
      "all_uses": ["*1:2:14"]
    }]
}
*/