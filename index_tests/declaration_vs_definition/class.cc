class Foo;
class Foo;
class Foo {};
class Foo;

/*
// NOTE: Separate decl/definition are not supported for classes. See source
//       for comments.
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
      "spell": "3:7-3:10|3:1-3:13|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": ["1:7-1:10|1:1-1:10|1|-1", "2:7-2:10|2:1-2:10|1|-1", "4:7-4:10|4:1-4:10|1|-1"],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
