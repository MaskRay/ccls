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
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "4:6-4:9|0|1|2",
      "extent": "4:1-7:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
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
      "spell": "2:8-2:23|0|1|2",
      "extent": "2:1-2:26|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [2580122838476012357],
      "uses": ["6:3-6:18|0|1|4"]
    }, {
      "usr": 13749354388332789217,
      "detailed_name": "struct ForwardType",
      "qual_name_offset": 7,
      "short_name": "ForwardType",
      "kind": 23,
      "declarations": ["1:8-1:19|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [16374832544037266261],
      "uses": ["5:3-5:14|0|1|4"]
    }],
  "usr2var": [{
      "usr": 2580122838476012357,
      "detailed_name": "ImplementedType b",
      "qual_name_offset": 16,
      "short_name": "b",
      "declarations": [],
      "spell": "6:19-6:20|4654328188330986029|3|2",
      "extent": "6:3-6:20|4654328188330986029|3|0",
      "type": 8508299082070213750,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 16374832544037266261,
      "detailed_name": "ForwardType *a",
      "qual_name_offset": 0,
      "short_name": "a",
      "declarations": [],
      "spell": "5:16-5:17|4654328188330986029|3|2",
      "extent": "5:3-5:17|4654328188330986029|3|0",
      "type": 13749354388332789217,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
