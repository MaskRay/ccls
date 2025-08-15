void foo(int a, int b);

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 2747674671862363334,
      "detailed_name": "void foo(int a, int b)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "bases": [],
      "vars": [8158338140950637730, 17005964293310927058],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["1:6-1:9|1:1-1:23|1|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 452,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [8158338140950637730, 17005964293310927058],
      "uses": []
    }],
  "usr2var": [{
      "usr": 8158338140950637730,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "spell": "1:14-1:15|1:10-1:15|1026|-1",
      "type": 452,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 17005964293310927058,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "spell": "1:21-1:22|1:17-1:22|1026|-1",
      "type": 452,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
