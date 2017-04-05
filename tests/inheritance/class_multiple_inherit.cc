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
      "definition": "1:7",
      "derived": [1, 2],
      "uses": ["*1:7", "*2:24", "*3:24"]
    }, {
      "id": 1,
      "usr": "c:@S@MiddleA",
      "short_name": "MiddleA",
      "qualified_name": "MiddleA",
      "definition": "2:7",
      "parents": [0],
      "derived": [3],
      "uses": ["*2:7", "*4:24"]
    }, {
      "id": 2,
      "usr": "c:@S@MiddleB",
      "short_name": "MiddleB",
      "qualified_name": "MiddleB",
      "definition": "3:7",
      "parents": [0],
      "derived": [3],
      "uses": ["*3:7", "*4:40"]
    }, {
      "id": 3,
      "usr": "c:@S@Derived",
      "short_name": "Derived",
      "qualified_name": "Derived",
      "definition": "4:7",
      "parents": [1, 2],
      "uses": ["*4:7"]
    }]
}
*/
