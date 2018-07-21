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
      "kind": 6,
      "storage": 0,
      "declarations": ["2:8-2:12|15041163540773201510|2|1025"],
      "spell": "5:11-5:15|15041163540773201510|2|1026",
      "extent": "5:1-8:2|15041163540773201510|2|0",
      "bases": [],
      "derived": [],
      "vars": [16380484338511689669],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [9488177941273031343],
      "vars": [],
      "instances": [16380484338511689669, 14455976355866885943],
      "uses": ["2:3-2:6|15041163540773201510|2|4", "5:1-5:4|0|1|4", "5:6-5:9|0|1|4", "6:3-6:6|9488177941273031343|3|4", "10:8-10:11|0|1|4"]
    }],
  "usr2var": [{
      "usr": 14455976355866885943,
      "detailed_name": "extern Foo foo",
      "qual_name_offset": 11,
      "short_name": "foo",
      "declarations": ["10:12-10:15|0|1|1"],
      "type": 15041163540773201510,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 16380484338511689669,
      "detailed_name": "Foo f",
      "qual_name_offset": 4,
      "short_name": "f",
      "declarations": [],
      "spell": "6:7-6:8|9488177941273031343|3|2",
      "extent": "6:3-6:8|9488177941273031343|3|0",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
