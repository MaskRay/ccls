struct Foo;

void foo() {
  Foo* a;
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 23,
      "declarations": ["1:8-1:11|-1|1|1"],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["4:3-4:6|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "3:6-3:9|-1|1|2",
      "extent": "3:1-5:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 13198746475679542317,
      "detailed_name": "Foo *a",
      "qual_name_offset": 5,
      "short_name": "a",
      "declarations": [],
      "spell": "4:8-4:9|0|3|2",
      "extent": "4:3-4:9|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
