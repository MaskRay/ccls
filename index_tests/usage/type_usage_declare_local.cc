struct ForwardType;
struct ImplementedType {};

void Foo() {
  ForwardType* a;
  ImplementedType b;
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 4654328188330986029,
      "detailed_name": "void Foo()",
      "qual_name_offset": 5,
      "short_name": "Foo",
      "spell": "4:6-4:9|4:1-7:2|2|-1",
      "bases": [],
      "vars": [16374832544037266261, 2580122838476012357],
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
      "instances": [2580122838476012357],
      "uses": ["6:3-6:18|4|-1"]
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
      "instances": [16374832544037266261],
      "uses": ["5:3-5:14|4|-1"]
    }],
  "usr2var": [{
      "usr": 2580122838476012357,
      "detailed_name": "ImplementedType b",
      "qual_name_offset": 16,
      "short_name": "b",
      "spell": "6:19-6:20|6:3-6:20|2|-1",
      "type": 8508299082070213750,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 16374832544037266261,
      "detailed_name": "ForwardType *a",
      "qual_name_offset": 13,
      "short_name": "a",
      "spell": "5:16-5:17|5:3-5:17|2|-1",
      "type": 13749354388332789217,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
