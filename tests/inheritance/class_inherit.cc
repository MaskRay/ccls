class Parent {};
class Derived : public Parent {};

/*
// TODO: Insert interesting usage for derived types. Maybe we should change out
//       interesting usage approach for types, and instead find a list of "uninteresting" usages.
//       Rather, what I think we should do is this
//
//      t -> interesting
//      f > uninteresting
//      fileid 0
//      row 5
//      column 7
//      this could all be packed into 64 bits
//      "usages": { "t@0:5:7" }
//
//    interesting: 1 bit   (2)
//    file:        29 bits (536,870,912)
//    line:        20 bits (1,048,576)
//    column:      14 bits (16,384)
//
//  When inserting a new usage, default to interesting, but if already present
//  don't flip it to uninteresting.
//
//  When importing we remap file ids.

OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Parent",
      "short_name": "Parent",
      "qualified_name": "Parent",
      "definition": "tests/inheritance/class_inherit.cc:1:7",
      "derived": [1],
      "all_uses": ["tests/inheritance/class_inherit.cc:1:7", "tests/inheritance/class_inherit.cc:2:24"],
      "interesting_uses": ["tests/inheritance/class_inherit.cc:2:24"]
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