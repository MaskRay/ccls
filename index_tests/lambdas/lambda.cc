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
  "usr2func": [{
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "1:6-1:9|0|1|2",
      "extent": "1:1-12:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [12666114896600231317, 2981279427664991319],
      "uses": [],
      "callees": ["9:14-9:15|17926497908620168464|3|32", "10:14-10:15|17926497908620168464|3|32", "11:14-11:15|17926497908620168464|3|32"]
    }, {
      "usr": 17926497908620168464,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["9:14-9:15|4259594751088586730|3|32", "10:14-10:15|4259594751088586730|3|32", "11:14-11:15|4259594751088586730|3|32"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 17,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [12666114896600231317, 12879188959314906706],
      "uses": []
    }, {
      "usr": 14635009347499519042,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [2981279427664991319],
      "uses": []
    }],
  "usr2var": [{
      "usr": 2981279427664991319,
      "detailed_name": "lambda dosomething",
      "qual_name_offset": 7,
      "short_name": "dosomething",
      "declarations": [],
      "spell": "4:8-4:19|4259594751088586730|3|2",
      "extent": "4:3-7:4|4259594751088586730|3|0",
      "type": 14635009347499519042,
      "uses": ["9:3-9:14|4259594751088586730|3|4", "10:3-10:14|4259594751088586730|3|4", "11:3-11:14|4259594751088586730|3|4"],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 12666114896600231317,
      "detailed_name": "int x",
      "qual_name_offset": 4,
      "short_name": "x",
      "declarations": [],
      "spell": "2:7-2:8|4259594751088586730|3|2",
      "extent": "2:3-2:8|4259594751088586730|3|0",
      "type": 17,
      "uses": ["5:7-5:8|0|1|4", "4:24-4:25|4259594751088586730|3|4"],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 12879188959314906706,
      "detailed_name": "int y",
      "qual_name_offset": 4,
      "short_name": "",
      "declarations": [],
      "spell": "4:31-4:32|4259594751088586730|3|2",
      "extent": "4:27-4:32|4259594751088586730|3|0",
      "type": 17,
      "uses": ["6:7-6:8|4259594751088586730|3|4"],
      "kind": 253,
      "storage": 1
    }]
}
*/
