class Root {};
class MiddleA : public Root {};
class MiddleB : public Root {};
class Derived : public MiddleA, public MiddleB {};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Root",
      "short_name": "Root",
      "qualified_name": "Root",
      "definition_spelling": "1:7-1:11",
      "definition_extent": "1:1-1:14",
      "derived": [1, 2],
      "uses": ["*1:7-1:11", "*2:24-2:28", "*3:24-3:28"]
    }, {
      "id": 1,
      "usr": "c:@S@MiddleA",
      "short_name": "MiddleA",
      "qualified_name": "MiddleA",
      "definition_spelling": "2:7-2:14",
      "definition_extent": "2:1-2:31",
      "parents": [0],
      "derived": [3],
      "uses": ["*2:7-2:14", "*4:24-4:31"]
    }, {
      "id": 2,
      "usr": "c:@S@MiddleB",
      "short_name": "MiddleB",
      "qualified_name": "MiddleB",
      "definition_spelling": "3:7-3:14",
      "definition_extent": "3:1-3:31",
      "parents": [0],
      "derived": [3],
      "uses": ["*3:7-3:14", "*4:40-4:47"]
    }, {
      "id": 3,
      "usr": "c:@S@Derived",
      "short_name": "Derived",
      "qualified_name": "Derived",
      "definition_spelling": "4:7-4:14",
      "definition_extent": "4:1-4:50",
      "parents": [1, 2],
      "uses": ["*4:7-4:14"]
    }]
}
*/
