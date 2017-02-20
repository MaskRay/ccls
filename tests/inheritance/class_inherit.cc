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
      "definition": "tests/inheritance/class_inherit.cc:1:7",
      "derived": [1],
      "all_uses": ["tests/inheritance/class_inherit.cc:1:7", "tests/inheritance/class_inherit.cc:2:24"]
    }, {
      "id": 1,
      "usr": "c:@S@Derived",
      "short_name": "Derived",
      "qualified_name": "Derived",
      "definition": "tests/inheritance/class_inherit.cc:2:7",
      "parents": [0],
      "all_uses": ["tests/inheritance/class_inherit.cc:2:7"]
    }],
  "functions": [],
  "variables": []
}
*/