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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "1:6-1:9|1:1-9:2|2|-1",
      "bases": [],
      "vars": [13311055950748663970, 14036425367303419504],
      "callees": [],
      "kind": 12,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 452,
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
      "instances": [13311055950748663970, 14036425367303419504],
      "uses": []
    }],
  "usr2var": [{
      "usr": 13311055950748663970,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "spell": "2:7-2:8|2:3-2:8|2|-1",
      "type": 452,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["3:3-3:4|20|-1", "8:3-8:4|20|-1"]
    }, {
      "usr": 14036425367303419504,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "spell": "5:9-5:10|5:5-5:10|2|-1",
      "type": 452,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["6:5-6:6|20|-1"]
    }]
}
*/
