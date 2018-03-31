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
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 5,
      "declarations": ["1:7-1:10|-1|1|1", "2:7-2:10|-1|1|1", "4:7-4:10|-1|1|1"],
      "spell": "3:7-3:10|-1|1|2",
      "extent": "3:1-3:13|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "funcs": [],
  "vars": []
}
*/
