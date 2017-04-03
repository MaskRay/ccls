template<typename T>
class Foo {};

Foo<int> a;
Foo<bool> b;

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "1:2:7",
      "instantiations": [0, 1],
      "uses": ["*1:2:7", "*1:4:1", "*1:5:1"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:4:10",
      "variable_type": 0,
      "uses": ["1:4:10"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition": "1:5:11",
      "variable_type": 0,
      "uses": ["1:5:11"]
    }]
}
*/
