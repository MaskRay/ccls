struct ForwardType;
void foo(ForwardType*) {}
/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 15327735280790448926,
      "detailed_name": "void foo(ForwardType *)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "2:6-2:9|2:1-2:26|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
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
      "instances": [],
      "uses": ["2:10-2:21|4|-1"]
    }],
  "usr2var": []
}
*/
