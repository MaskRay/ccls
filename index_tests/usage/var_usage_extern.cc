extern int a;

void foo() {
  a = 5;
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
      "spell": "3:6-3:9|0|1|2",
      "extent": "3:1-5:2|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
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
      "instances": [16721564935990383768],
      "uses": []
    }],
  "usr2var": [{
      "usr": 16721564935990383768,
      "detailed_name": "extern int a",
      "qual_name_offset": 11,
      "short_name": "a",
      "declarations": ["1:12-1:13|0|1|1"],
      "type": 53,
      "uses": ["4:3-4:4|4259594751088586730|3|20"],
      "kind": 13,
      "storage": 1
    }]
}
*/
