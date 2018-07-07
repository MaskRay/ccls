class Foo;
class Foo;
class Foo {};
class Foo;

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "class Foo",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": ["1:7-1:10|0|1|1", "2:7-2:10|0|1|1", "4:7-4:10|0|1|1"],
      "spell": "3:7-3:10|0|1|2",
      "extent": "1:1-1:10|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
