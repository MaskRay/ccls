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
      "definition": "1:12",
      "vars": [0, 1],
      "instantiations": [2],
      "uses": ["*1:12", "*6:1", "6:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@E@Foo@A",
      "short_name": "A",
      "qualified_name": "Foo::A",
      "definition": "2:3",
      "variable_type": 0,
      "declaring_type": 0,
      "uses": ["2:3", "6:14"]
    }, {
      "id": 1,
      "usr": "c:@E@Foo@B",
      "short_name": "B",
      "qualified_name": "Foo::B",
      "definition": "3:3",
      "variable_type": 0,
      "declaring_type": 0,
      "uses": ["3:3"]
    }, {
      "id": 2,
      "usr": "c:@x",
      "short_name": "x",
      "qualified_name": "x",
      "definition": "6:5",
      "variable_type": 0,
      "uses": ["6:5"]
    }]
}
*/
