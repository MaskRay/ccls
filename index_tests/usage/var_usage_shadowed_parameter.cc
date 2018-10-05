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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 11998306017310352355,
      "detailed_name": "void foo(int a)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "1:6-1:9|1:1-8:2|2|-1",
      "bases": [],
      "vars": [11608231465452906059, 6997229590862003559],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
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
      "instances": [11608231465452906059, 6997229590862003559],
      "uses": []
    }],
  "usr2var": [{
      "usr": 6997229590862003559,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "spell": "4:9-4:10|4:5-4:10|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["5:5-5:6|20|-1"]
    }, {
      "usr": 11608231465452906059,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "spell": "1:14-1:15|1:10-1:15|1026|-1",
      "type": 53,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["2:3-2:4|20|-1", "7:3-7:4|20|-1"]
    }]
}
*/
