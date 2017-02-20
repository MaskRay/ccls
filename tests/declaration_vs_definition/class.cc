class Foo;
class Foo;
class Foo {};
class Foo;

/*
// NOTE: Separate decl/definition are not supported for classes. See source
//       for comments.
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/declaration_vs_definition/class.cc:3:7",
      "all_uses": ["tests/declaration_vs_definition/class.cc:1:7", "tests/declaration_vs_definition/class.cc:2:7", "tests/declaration_vs_definition/class.cc:3:7", "tests/declaration_vs_definition/class.cc:4:7"]
    }],
  "functions": [],
  "variables": []
}
*/