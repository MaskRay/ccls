struct Foo;

void foo() {
  Foo* a;
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
      "vars": [13198746475679542317],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": ["1:8-1:11|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [13198746475679542317],
      "uses": ["4:3-4:6|4259594751088586730|3|4"]
    }],
  "usr2var": [{
      "usr": 13198746475679542317,
      "detailed_name": "Foo *a",
      "qual_name_offset": 5,
      "short_name": "a",
      "declarations": [],
      "spell": "4:8-4:9|4259594751088586730|3|2",
      "extent": "4:3-4:9|4259594751088586730|3|0",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
