static int a;

void foo() {
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
      "instances": [11823161916242867318],
      "uses": []
    }],
  "usr2var": [{
      "usr": 11823161916242867318,
      "detailed_name": "static int a",
      "qual_name_offset": 11,
      "short_name": "a",
      "spell": "1:12-1:13|1:1-1:13|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 0,
      "storage": 2,
      "declarations": [],
      "uses": ["4:3-4:4|20|-1"]
    }]
}
*/
