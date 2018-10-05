class Foo {
public:
  Foo() {}
};

void foo() {
  Foo f;
  Foo* f2 = new Foo();
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 3385168158331140247,
      "detailed_name": "Foo::Foo()",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "spell": "3:3-3:6|3:3-3:11|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 9,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["7:7-7:8|16676|-1", "8:17-8:20|16676|-1"]
    }, {
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "6:6-6:9|6:1-9:2|2|-1",
      "bases": [],
      "vars": [10983126130596230582, 17165811951126099095],
      "callees": ["7:7-7:8|3385168158331140247|3|16676", "7:7-7:8|3385168158331140247|3|16676", "8:17-8:20|3385168158331140247|3|16676", "8:17-8:20|3385168158331140247|3|16676"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "spell": "1:7-1:10|1:1-4:2|2|-1",
      "bases": [],
      "funcs": [3385168158331140247],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [10983126130596230582, 17165811951126099095],
      "uses": ["3:3-3:6|4|-1", "7:3-7:6|4|-1", "8:3-8:6|4|-1", "8:17-8:20|4|-1"]
    }],
  "usr2var": [{
      "usr": 10983126130596230582,
      "detailed_name": "Foo f",
      "qual_name_offset": 4,
      "short_name": "f",
      "spell": "7:7-7:8|7:3-7:8|2|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 17165811951126099095,
      "detailed_name": "Foo *f2",
      "qual_name_offset": 5,
      "short_name": "f2",
      "hover": "Foo *f2 = new Foo()",
      "spell": "8:8-8:10|8:3-8:22|2|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
