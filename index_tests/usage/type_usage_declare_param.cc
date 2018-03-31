struct ForwardType;
struct ImplementedType {};

void foo(ForwardType* f, ImplementedType a) {}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 13749354388332789217,
      "detailed_name": "ForwardType",
      "short_name": "ForwardType",
      "kind": 23,
      "declarations": ["1:8-1:19|-1|1|1"],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["4:10-4:21|-1|1|4"]
    }, {
      "id": 1,
      "usr": 8508299082070213750,
      "detailed_name": "ImplementedType",
      "short_name": "ImplementedType",
      "kind": 23,
      "declarations": [],
      "spell": "2:8-2:23|-1|1|2",
      "extent": "2:1-2:26|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [1],
      "uses": ["4:26-4:41|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 1699390678058422036,
      "detailed_name": "void foo(ForwardType *f, ImplementedType a)",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "4:6-4:9|-1|1|2",
      "extent": "4:1-4:47|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0, 1],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 2584795197111552890,
      "detailed_name": "ForwardType *f",
      "short_name": "f",
      "declarations": [],
      "spell": "4:23-4:24|0|3|2",
      "extent": "4:10-4:24|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 253,
      "storage": 1
    }, {
      "id": 1,
      "usr": 5136230284979460117,
      "detailed_name": "ImplementedType a",
      "short_name": "a",
      "declarations": [],
      "spell": "4:42-4:43|0|3|2",
      "extent": "4:26-4:43|0|3|0",
      "type": 1,
      "uses": [],
      "kind": 253,
      "storage": 1
    }]
}
*/
