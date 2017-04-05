class Parent {};
class Derived : public Parent {};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Parent",
      "short_name": "Parent",
      "qualified_name": "Parent",
      "definition_spelling": "1:7-1:13",
      "definition_extent": "1:1-1:16",
      "derived": [1],
      "uses": ["*1:7-1:13", "*2:24-2:30"]
    }, {
      "id": 1,
      "usr": "c:@S@Derived",
      "short_name": "Derived",
      "qualified_name": "Derived",
      "definition_spelling": "2:7-2:14",
      "definition_extent": "2:1-2:33",
      "parents": [0],
      "uses": ["*2:7-2:14"]
    }]
}
*/
