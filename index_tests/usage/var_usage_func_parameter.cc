void foo(int a) {
  a += 10;
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
      "instances": [0],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 11998306017310352355,
      "detailed_name": "void foo(int a)",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "1:6-1:9|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 10063793875496522529,
      "detailed_name": "int a",
      "short_name": "a",
      "declarations": [],
      "spell": "1:14-1:15|0|3|2",
      "extent": "1:10-1:15|0|3|0",
      "type": 0,
      "uses": ["2:3-2:4|0|3|4"],
      "kind": 253,
      "storage": 1
    }]
}
*/
