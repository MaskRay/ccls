class Foo {
  static int foo;
};

int Foo::foo;

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
      "uses": ["*1:1:7", "1:5:5"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@foo",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "declaration": "1:2:14",
      "definition": "1:5:10",
      "declaring_type": 0,
      "uses": ["1:2:14", "1:5:10"]
    }]
}
*/