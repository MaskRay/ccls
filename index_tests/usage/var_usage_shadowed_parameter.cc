void foo(int a) {
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
      "usr": 11998306017310352355,
      "detailed_name": "void foo(int a)",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "1:6-1:9|-1|1|2",
      "extent": "1:1-8:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0, 1],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 11608231465452906059,
      "detailed_name": "int a",
      "short_name": "a",
      "declarations": [],
      "spell": "1:14-1:15|0|3|2",
      "extent": "1:10-1:15|0|3|0",
      "type": 0,
      "uses": ["2:3-2:4|0|3|4", "7:3-7:4|0|3|4"],
      "kind": 253,
      "storage": 1
    }, {
      "id": 1,
      "usr": 8011559936501990179,
      "detailed_name": "int a",
      "short_name": "a",
      "declarations": [],
      "spell": "4:9-4:10|0|3|2",
      "extent": "4:5-4:10|0|3|0",
      "type": 0,
      "uses": ["5:5-5:6|0|3|4"],
      "kind": 13,
      "storage": 1
    }]
}
*/
