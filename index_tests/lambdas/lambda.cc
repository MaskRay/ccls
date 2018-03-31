void foo() {
  int x;

  auto dosomething = [&x](int y) {
    ++x;
    ++y;
  };

  dosomething(1);
  dosomething(1);
  dosomething(1);
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
      "instances": [0, 2],
      "uses": []
    }, {
      "id": 1,
      "usr": 1287417953265234030,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [1],
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
      "extent": "1:1-12:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0, 1],
      "uses": [],
      "callees": ["9:14-9:15|1|3|32", "10:14-10:15|1|3|32", "11:14-11:15|1|3|32"]
    }, {
      "id": 1,
      "usr": 1328781044864682611,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["9:14-9:15|0|3|32", "10:14-10:15|0|3|32", "11:14-11:15|0|3|32"],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 17270098654620601683,
      "detailed_name": "int x",
      "short_name": "x",
      "declarations": [],
      "spell": "2:7-2:8|0|3|2",
      "extent": "2:3-2:8|0|3|0",
      "type": 0,
      "uses": ["5:7-5:8|-1|1|4", "4:24-4:25|0|3|4"],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 16806544259835773270,
      "detailed_name": "lambda dosomething",
      "short_name": "dosomething",
      "declarations": [],
      "spell": "4:8-4:19|0|3|2",
      "extent": "4:3-7:4|0|3|0",
      "type": 1,
      "uses": ["9:3-9:14|0|3|4", "10:3-10:14|0|3|4", "11:3-11:14|0|3|4"],
      "kind": 13,
      "storage": 1
    }, {
      "id": 2,
      "usr": 2034725908368218782,
      "detailed_name": "int y",
      "short_name": "y",
      "declarations": [],
      "spell": "4:31-4:32|0|3|2",
      "extent": "4:27-4:32|0|3|0",
      "type": 0,
      "uses": ["6:7-6:8|0|3|4"],
      "kind": 253,
      "storage": 1
    }]
}
*/
