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
      "definition": "1:3:7",
      "all_uses": ["1:1:7", "1:2:7", "1:3:7", "1:4:7"]
    }],
  "functions": [],
  "variables": []
}
*/