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
      "spell": "3:6-3:9|3:1-5:2|2|-1",
      "bases": [],
      "vars": [],
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
      "instances": [16721564935990383768],
      "uses": []
    }],
  "usr2var": [{
      "usr": 16721564935990383768,
      "detailed_name": "extern int a",
      "qual_name_offset": 11,
      "short_name": "a",
      "type": 53,
      "kind": 13,
      "parent_kind": 0,
      "storage": 1,
      "declarations": ["1:12-1:13|1:1-1:13|1|-1"],
      "uses": ["4:3-4:4|20|-1"]
    }]
}
*/
