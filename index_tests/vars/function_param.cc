struct Foo;

void foo(Foo* p0, Foo* p1) {}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 8908726657907936744,
      "detailed_name": "void foo(Foo *p0, Foo *p1)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "3:6-3:9|0|1|2",
      "extent": "3:1-3:30|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [8730439006497971620, 2525014371090380500],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 23,
      "declarations": ["1:8-1:11|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [8730439006497971620, 2525014371090380500],
      "uses": ["3:10-3:13|0|1|4", "3:19-3:22|0|1|4"]
    }],
  "usr2var": [{
      "usr": 2525014371090380500,
      "detailed_name": "Foo *p1",
      "qual_name_offset": 5,
      "short_name": "p1",
      "declarations": [],
      "spell": "3:24-3:26|8908726657907936744|3|2",
      "extent": "3:19-3:26|8908726657907936744|3|0",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 253,
      "storage": 0
    }, {
      "usr": 8730439006497971620,
      "detailed_name": "Foo *p0",
      "qual_name_offset": 5,
      "short_name": "p0",
      "declarations": [],
      "spell": "3:15-3:17|8908726657907936744|3|2",
      "extent": "3:10-3:17|8908726657907936744|3|0",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 253,
      "storage": 0
    }]
}
*/
