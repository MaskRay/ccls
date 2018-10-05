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
      "spell": "4:6-4:9|4:1-4:47|2|-1",
      "bases": [],
      "vars": [13058491096576226774, 11055777568039014776],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 8508299082070213750,
      "detailed_name": "struct ImplementedType {}",
      "qual_name_offset": 7,
      "short_name": "ImplementedType",
      "spell": "2:8-2:23|2:1-2:26|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [11055777568039014776],
      "uses": ["4:26-4:41|4|-1"]
    }, {
      "usr": 13749354388332789217,
      "detailed_name": "struct ForwardType",
      "qual_name_offset": 7,
      "short_name": "ForwardType",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": ["1:8-1:19|1:1-1:19|1|-1"],
      "derived": [],
      "instances": [13058491096576226774],
      "uses": ["4:10-4:21|4|-1"]
    }],
  "usr2var": [{
      "usr": 11055777568039014776,
      "detailed_name": "ImplementedType a",
      "qual_name_offset": 16,
      "short_name": "a",
      "spell": "4:42-4:43|4:26-4:43|1026|-1",
      "type": 8508299082070213750,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 13058491096576226774,
      "detailed_name": "ForwardType *f",
      "qual_name_offset": 13,
      "short_name": "f",
      "spell": "4:23-4:24|4:10-4:24|1026|-1",
      "type": 13749354388332789217,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
