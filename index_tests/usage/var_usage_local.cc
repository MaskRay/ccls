void foo() {
  int x;
  x = 3;
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
      "spell": "1:6-1:9|1:1-4:2|2|-1",
      "bases": [],
      "vars": [14014650769929566957],
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
      "instances": [14014650769929566957],
      "uses": []
    }],
  "usr2var": [{
      "usr": 14014650769929566957,
      "detailed_name": "int x",
      "qual_name_offset": 4,
      "short_name": "x",
      "spell": "2:7-2:8|2:3-2:8|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["3:3-3:4|20|-1"]
    }]
}
*/
