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
      "detailed_name": "Foo",
      "definition_spelling": "3:7-3:10",
      "definition_extent": "3:1-3:13",
      "uses": ["1:7-1:10", "2:7-2:10", "3:7-3:10", "4:7-4:10"]
    }]
}
*/
