void called() {}

struct Foo {
  Foo();
};

Foo::Foo() {
  called();
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 468307235068920063,
      "detailed_name": "void called()",
      "qual_name_offset": 5,
      "short_name": "called",
      "spell": "1:6-1:12|1:1-1:17|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["8:3-8:9|16420|-1"]
    }, {
      "usr": 3385168158331140247,
      "detailed_name": "Foo::Foo()",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "spell": "7:6-7:9|7:1-9:2|1026|-1",
      "bases": [],
      "vars": [],
      "callees": ["8:3-8:9|468307235068920063|3|16420"],
      "kind": 9,
      "parent_kind": 23,
      "storage": 0,
      "declarations": ["4:3-4:6|4:3-4:8|1025|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "3:8-3:11|3:1-5:2|2|-1",
      "bases": [],
      "funcs": [3385168158331140247],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["4:3-4:6|4|-1", "7:1-7:4|4|-1", "7:6-7:9|4|-1"]
    }],
  "usr2var": []
}
*/
