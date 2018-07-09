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
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "1:6-1:9|0|1|2",
      "extent": "1:1-9:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [13311055950748663970, 14036425367303419504],
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
      "instances": [13311055950748663970, 14036425367303419504],
      "uses": []
    }],
  "usr2var": [{
      "usr": 13311055950748663970,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "declarations": [],
      "spell": "2:7-2:8|4259594751088586730|3|2",
      "extent": "2:3-2:8|4259594751088586730|3|0",
      "type": 53,
      "uses": ["3:3-3:4|4259594751088586730|3|20", "8:3-8:4|4259594751088586730|3|20"],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 14036425367303419504,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "declarations": [],
      "spell": "5:9-5:10|4259594751088586730|3|2",
      "extent": "5:5-5:10|4259594751088586730|3|0",
      "type": 53,
      "uses": ["6:5-6:6|4259594751088586730|3|20"],
      "kind": 13,
      "storage": 0
    }]
}
*/
