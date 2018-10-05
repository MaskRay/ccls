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
      "spell": "3:6-3:9|3:1-5:2|2|-1",
      "bases": [],
      "vars": [13198746475679542317],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": ["1:8-1:11|1:1-1:11|1|-1"],
      "derived": [],
      "instances": [13198746475679542317],
      "uses": ["4:3-4:6|4|-1"]
    }],
  "usr2var": [{
      "usr": 13198746475679542317,
      "detailed_name": "Foo *a",
      "qual_name_offset": 5,
      "short_name": "a",
      "spell": "4:8-4:9|4:3-4:9|2|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
