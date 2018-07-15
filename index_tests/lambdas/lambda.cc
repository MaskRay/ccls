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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "1:6-1:9|0|1|2",
      "extent": "1:1-12:2|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [12666114896600231317, 2981279427664991319],
      "uses": [],
      "callees": []
    }, {
      "usr": 17926497908620168464,
      "detailed_name": "inline void foo()::(anon class)::operator()(int y) const",
      "qual_name_offset": 12,
      "short_name": "operator()",
      "kind": 6,
      "storage": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["9:14-9:15|14635009347499519042|2|16420", "10:14-10:15|14635009347499519042|2|16420", "11:14-11:15|14635009347499519042|2|16420"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 53,
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
      "instances": [12666114896600231317],
      "uses": []
    }, {
      "usr": 14635009347499519042,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 26,
      "declarations": [],
      "spell": "4:22-4:23|4259594751088586730|3|2",
      "extent": "4:22-4:23|4259594751088586730|3|0",
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
      "detailed_name": "(lambda) dosomething",
      "qual_name_offset": 9,
      "short_name": "dosomething",
      "hover": "(lambda) dosomething",
      "declarations": [],
      "spell": "4:8-4:19|4259594751088586730|3|2",
      "extent": "4:3-7:4|4259594751088586730|3|0",
      "type": 14635009347499519042,
      "uses": ["9:3-9:14|4259594751088586730|3|4", "10:3-10:14|4259594751088586730|3|4", "11:3-11:14|4259594751088586730|3|4"],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 12666114896600231317,
      "detailed_name": "int x",
      "qual_name_offset": 4,
      "short_name": "x",
      "declarations": [],
      "spell": "2:7-2:8|4259594751088586730|3|2",
      "extent": "2:3-2:8|4259594751088586730|3|0",
      "type": 53,
      "uses": ["4:24-4:25|4259594751088586730|3|4", "5:7-5:8|4259594751088586730|3|28"],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 12879188959314906706,
      "detailed_name": "int y",
      "qual_name_offset": 4,
      "short_name": "y",
      "declarations": [],
      "spell": "4:31-4:32|17926497908620168464|3|2",
      "extent": "4:27-4:32|17926497908620168464|3|0",
      "type": 0,
      "uses": ["6:7-6:8|17926497908620168464|3|28"],
      "kind": 253,
      "storage": 0
    }]
}
*/
