void foo() {
  int a;
  a = 1;
  {
    int a;
    a = 2;
  }
  a = 3;
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
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "1:6-1:9|-1|1|2",
      "extent": "1:1-9:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0, 1],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 3440226937504376525,
      "detailed_name": "int a",
      "short_name": "a",
      "declarations": [],
      "spell": "2:7-2:8|0|3|2",
      "extent": "2:3-2:8|0|3|0",
      "type": 0,
      "uses": ["3:3-3:4|0|3|4", "8:3-8:4|0|3|4"],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 14700715011944976607,
      "detailed_name": "int a",
      "short_name": "a",
      "declarations": [],
      "spell": "5:9-5:10|0|3|2",
      "extent": "5:5-5:10|0|3|0",
      "type": 0,
      "uses": ["6:5-6:6|0|3|4"],
      "kind": 13,
      "storage": 1
    }]
}
*/
