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
      "qualified_name": "Foo",
      "definition": "*1:1:7",
      "vars": [0],
      "all_uses": ["*1:1:7"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@foo",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "definition": "*1:2:7",
      "declaring_type": 0,
      "all_uses": ["*1:2:7"]
    }]
}
*/