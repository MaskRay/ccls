class Foo;
class Foo;
class Foo {};
class Foo;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "3:7-3:10",
      "definition_extent": "3:1-3:13",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["1:7-1:10", "2:7-2:10", "3:7-3:10", "4:7-4:10"]
    }],
  "funcs": [],
  "vars": []
}
*/
