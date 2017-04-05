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
      "definition_spelling": "2:7-2:10",
      "definition_extent": "2:1-2:13",
      "instantiations": [0, 1],
      "uses": ["*2:7-2:10", "*4:1-4:4", "*5:1-5:4"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition_spelling": "4:10-4:11",
      "definition_extent": "4:1-4:11",
      "variable_type": 0,
      "uses": ["4:10-4:11"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition_spelling": "5:11-5:12",
      "definition_extent": "5:1-5:12",
      "variable_type": 0,
      "uses": ["5:11-5:12"]
    }]
}
*/
