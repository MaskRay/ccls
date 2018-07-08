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
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "1:6-1:9|0|1|2",
      "extent": "1:1-4:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [14014650769929566957],
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
      "instances": [14014650769929566957],
      "uses": []
    }],
  "usr2var": [{
      "usr": 14014650769929566957,
      "detailed_name": "int x",
      "qual_name_offset": 4,
      "short_name": "x",
      "declarations": [],
      "spell": "2:7-2:8|4259594751088586730|3|2",
      "extent": "2:3-2:8|4259594751088586730|3|0",
      "type": 53,
      "uses": ["3:3-3:4|4259594751088586730|3|20"],
      "kind": 13,
      "storage": 0
    }]
}
*/
