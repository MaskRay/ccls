class Foo {
  Foo* make();
};

Foo* Foo::make() {
  Foo f;
  return nullptr;
}

extern Foo foo;

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 9488177941273031343,
      "detailed_name": "Foo *Foo::make()",
      "qual_name_offset": 5,
      "short_name": "make",
      "spell": "5:11-5:15|5:1-8:2|1026|-1",
      "bases": [],
      "vars": [16380484338511689669],
      "callees": [],
      "kind": 6,
      "parent_kind": 5,
      "storage": 0,
      "declarations": ["2:8-2:12|2:3-2:14|1025|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "spell": "1:7-1:10|1:1-3:2|2|-1",
      "bases": [],
      "funcs": [9488177941273031343],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [16380484338511689669, 14455976355866885943],
      "uses": ["2:3-2:6|4|-1", "5:1-5:4|4|-1", "5:6-5:9|4|-1", "6:3-6:6|4|-1", "10:8-10:11|4|-1"]
    }],
  "usr2var": [{
      "usr": 14455976355866885943,
      "detailed_name": "extern Foo foo",
      "qual_name_offset": 11,
      "short_name": "foo",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 0,
      "storage": 1,
      "declarations": ["10:12-10:15|10:1-10:15|1|-1"],
      "uses": []
    }, {
      "usr": 16380484338511689669,
      "detailed_name": "Foo f",
      "qual_name_offset": 4,
      "short_name": "f",
      "spell": "6:7-6:8|6:3-6:8|2|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 6,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
