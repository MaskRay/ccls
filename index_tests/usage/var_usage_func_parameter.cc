void foo(int a) {
  a += 10;
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
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "1:6-1:9|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [10063793875496522529],
      "uses": [],
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
      "instances": [10063793875496522529],
      "uses": []
    }],
  "usr2var": [{
      "usr": 10063793875496522529,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "declarations": [],
      "spell": "1:14-1:15|11998306017310352355|3|1026",
      "extent": "1:10-1:15|11998306017310352355|3|0",
      "type": 53,
      "uses": ["2:3-2:4|11998306017310352355|3|4"],
      "kind": 253,
      "storage": 0
    }]
}
*/
