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
      "kind": 12,
      "storage": 0,
      "declarations": ["3:6-3:9|0|1|1"],
      "spell": "4:6-4:9|0|1|2",
      "extent": "4:1-4:26|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [13823260660189154978],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": ["1:8-1:11|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [13823260660189154978],
      "uses": ["3:10-3:13|0|1|4", "3:18-3:21|0|1|4", "4:10-4:13|0|1|4", "4:18-4:21|0|1|4"]
    }],
  "usr2var": [{
      "usr": 13823260660189154978,
      "detailed_name": "Foo *f",
      "qual_name_offset": 5,
      "short_name": "f",
      "declarations": [],
      "spell": "4:15-4:16|8908726657907936744|3|1026",
      "extent": "4:10-4:16|8908726657907936744|3|0",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 253,
      "storage": 0
    }]
}
*/
