template<typename T>
class Foo {};

Foo<int> a;
Foo<bool> b;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "2:7-2:10",
      "definition_extent": "2:1-2:13",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1],
      "uses": ["2:7-2:10", "4:1-4:4", "5:1-5:4"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "detailed_name": "Foo<int> a",
      "definition_spelling": "4:10-4:11",
      "definition_extent": "4:1-4:11",
      "variable_type": 0,
      "cls": 3,
      "uses": ["4:10-4:11"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "detailed_name": "Foo<bool> b",
      "definition_spelling": "5:11-5:12",
      "definition_extent": "5:1-5:12",
      "variable_type": 0,
      "cls": 3,
      "uses": ["5:11-5:12"]
    }]
}
*/
