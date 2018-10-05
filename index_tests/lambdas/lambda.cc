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
      "spell": "1:6-1:9|1:1-12:2|2|-1",
      "bases": [],
      "vars": [12666114896600231317, 2981279427664991319],
      "callees": ["9:14-9:15|17926497908620168464|3|16420", "10:14-10:15|17926497908620168464|3|16420", "11:14-11:15|17926497908620168464|3|16420"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 17926497908620168464,
      "detailed_name": "inline void foo()::(anon class)::operator()(int y) const",
      "qual_name_offset": 12,
      "short_name": "operator()",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["9:14-9:15|16420|-1", "10:14-10:15|16420|-1", "11:14-11:15|16420|-1"]
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [12666114896600231317],
      "uses": []
    }, {
      "usr": 14635009347499519042,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [2981279427664991319],
      "uses": []
    }],
  "usr2var": [{
      "usr": 2981279427664991319,
      "detailed_name": "(lambda) dosomething",
      "qual_name_offset": 9,
      "short_name": "dosomething",
      "hover": "(lambda) dosomething",
      "spell": "4:8-4:19|4:3-7:4|2|-1",
      "type": 14635009347499519042,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["9:3-9:14|4|-1", "10:3-10:14|4|-1", "11:3-11:14|4|-1"]
    }, {
      "usr": 12666114896600231317,
      "detailed_name": "int x",
      "qual_name_offset": 4,
      "short_name": "x",
      "spell": "2:7-2:8|2:3-2:8|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["4:24-4:25|4|-1", "5:7-5:8|28|-1"]
    }, {
      "usr": 12879188959314906706,
      "detailed_name": "int y",
      "qual_name_offset": 4,
      "short_name": "y",
      "spell": "4:31-4:32|4:27-4:32|2|-1",
      "type": 0,
      "kind": 253,
      "parent_kind": 6,
      "storage": 0,
      "declarations": [],
      "uses": ["6:7-6:8|28|-1"]
    }]
}
*/
