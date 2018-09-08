struct ForwardType;
struct ImplementedType {};

void foo(ForwardType* f, ImplementedType a) {}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 1699390678058422036,
      "detailed_name": "void foo(ForwardType *f, ImplementedType a)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "4:6-4:9|0|1|2|-1",
      "extent": "4:1-4:47|0|1|0|-1",
      "bases": [],
      "derived": [],
      "vars": [13058491096576226774, 11055777568039014776],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 8508299082070213750,
      "detailed_name": "struct ImplementedType {}",
      "qual_name_offset": 7,
      "short_name": "ImplementedType",
      "kind": 23,
      "declarations": [],
      "spell": "2:8-2:23|0|1|2|-1",
      "extent": "2:1-2:26|0|1|0|-1",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [11055777568039014776],
      "uses": ["4:26-4:41|0|1|4|-1"]
    }, {
      "usr": 13749354388332789217,
      "detailed_name": "struct ForwardType",
      "qual_name_offset": 7,
      "short_name": "ForwardType",
      "kind": 23,
      "declarations": ["1:8-1:19|1:1-1:19|0|1|1|-1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [13058491096576226774],
      "uses": ["4:10-4:21|0|1|4|-1"]
    }],
  "usr2var": [{
      "usr": 11055777568039014776,
      "detailed_name": "ImplementedType a",
      "qual_name_offset": 16,
      "short_name": "a",
      "declarations": [],
      "spell": "4:42-4:43|1699390678058422036|3|1026|-1",
      "extent": "4:26-4:43|1699390678058422036|3|0|-1",
      "type": 8508299082070213750,
      "uses": [],
      "kind": 253,
      "storage": 0
    }, {
      "usr": 13058491096576226774,
      "detailed_name": "ForwardType *f",
      "qual_name_offset": 13,
      "short_name": "f",
      "declarations": [],
      "spell": "4:23-4:24|1699390678058422036|3|1026|-1",
      "extent": "4:10-4:24|1699390678058422036|3|0|-1",
      "type": 13749354388332789217,
      "uses": [],
      "kind": 253,
      "storage": 0
    }]
}
*/
