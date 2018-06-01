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
  "usr2func": [{
      "usr": 11998306017310352355,
      "detailed_name": "void foo(int a)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "1:6-1:9|0|1|2",
      "extent": "1:1-8:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [11608231465452906059, 6997229590862003559],
      "uses": [],
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
      "instances": [11608231465452906059, 6997229590862003559],
      "uses": []
    }],
  "usr2var": [{
      "usr": 6997229590862003559,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "declarations": [],
      "spell": "4:9-4:10|11998306017310352355|3|2",
      "extent": "4:5-4:10|11998306017310352355|3|0",
      "type": 17,
      "uses": ["5:5-5:6|11998306017310352355|3|4"],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 11608231465452906059,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "declarations": [],
      "spell": "1:14-1:15|11998306017310352355|3|2",
      "extent": "1:10-1:15|11998306017310352355|3|0",
      "type": 17,
      "uses": ["2:3-2:4|11998306017310352355|3|4", "7:3-7:4|11998306017310352355|3|4"],
      "kind": 253,
      "storage": 0
    }]
}
*/
