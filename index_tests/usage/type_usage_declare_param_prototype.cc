struct Foo;

void foo(Foo* f, Foo*);
void foo(Foo* f, Foo*) {}

/*
// TODO: No interesting usage on prototype. But maybe that's ok!
// TODO: We should have the same variable declared for both prototype and
//       declaration. So it should have a usage marker on both. Then we could
//       rename parameters!

OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 8908726657907936744,
      "detailed_name": "void foo(Foo *f, Foo *)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "4:6-4:9|4:1-4:26|2|-1",
      "bases": [],
      "vars": [13823260660189154978],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["3:6-3:9|3:1-3:23|1|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": ["1:8-1:11|1:1-1:11|1|-1"],
      "derived": [],
      "instances": [13823260660189154978],
      "uses": ["3:10-3:13|4|-1", "3:18-3:21|4|-1", "4:10-4:13|4|-1", "4:18-4:21|4|-1"]
    }],
  "usr2var": [{
      "usr": 13823260660189154978,
      "detailed_name": "Foo *f",
      "qual_name_offset": 5,
      "short_name": "f",
      "spell": "4:15-4:16|4:10-4:16|1026|-1",
      "type": 15041163540773201510,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
