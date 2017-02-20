class Foo;
class Foo;
class Foo {};
class Foo;

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/class_forward_declaration.cc:3:7",
      "all_uses": ["tests/class_forward_declaration.cc:1:7", "tests/class_forward_declaration.cc:2:7", "tests/class_forward_declaration.cc:3:7", "tests/class_forward_declaration.cc:4:7"]
    }],
  "functions": [],
  "variables": []
}
*/