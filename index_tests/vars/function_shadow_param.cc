void foo(int p) {
  { int p = 0; }
}
/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 17,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 11998306017310352355,
      "detailed_name": "void foo(int p)",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "1:6-1:9|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0, 1],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 5875271969926422921,
      "detailed_name": "int p",
      "short_name": "p",
      "declarations": [],
      "spell": "1:14-1:15|0|3|2",
      "extent": "1:10-1:15|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 253,
      "storage": 1
    }, {
      "id": 1,
      "usr": 2147918703972955240,
      "detailed_name": "int p",
      "short_name": "p",
      "hover": "int p = 0",
      "declarations": [],
      "spell": "2:9-2:10|0|3|2",
      "extent": "2:5-2:14|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
