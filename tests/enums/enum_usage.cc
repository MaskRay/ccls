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
      "qualified_name": "Foo",
      "definition": "1:1:12",
      "vars": [0, 1],
      "uses": ["*1:1:12", "*1:6:1", "1:6:9"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@E@Foo@A",
      "short_name": "A",
      "qualified_name": "Foo::A",
      "definition": "1:2:3",
      "declaring_type": 0,
      "uses": ["1:2:3", "1:6:14"]
    }, {
      "id": 1,
      "usr": "c:@E@Foo@B",
      "short_name": "B",
      "qualified_name": "Foo::B",
      "definition": "1:3:3",
      "declaring_type": 0,
      "uses": ["1:3:3"]
    }, {
      "id": 2,
      "usr": "c:@x",
      "short_name": "x",
      "qualified_name": "x",
      "definition": "1:6:5",
      "variable_type": 0,
      "uses": ["1:6:5"]
    }]
}
*/