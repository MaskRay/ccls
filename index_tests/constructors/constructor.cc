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
      "kind": 9,
      "storage": 0,
      "declarations": [],
      "spell": "3:3-3:6|15041163540773201510|2|514",
      "extent": "3:3-3:11|15041163540773201510|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["7:7-7:8|15041163540773201510|2|8228", "8:17-8:20|15041163540773201510|2|8228"],
      "callees": []
    }, {
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "6:6-6:9|0|1|2",
      "extent": "6:1-9:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|0|1|2",
      "extent": "1:1-4:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [3385168158331140247],
      "vars": [],
      "instances": [10983126130596230582, 17165811951126099095],
      "uses": ["3:3-3:6|0|1|4", "7:3-7:6|0|1|4", "8:3-8:6|0|1|4", "8:17-8:20|0|1|4"]
    }],
  "usr2var": [{
      "usr": 10983126130596230582,
      "detailed_name": "Foo f",
      "qual_name_offset": 4,
      "short_name": "f",
      "declarations": [],
      "spell": "7:7-7:8|4259594751088586730|3|2",
      "extent": "7:3-7:8|4259594751088586730|3|0",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 17165811951126099095,
      "detailed_name": "Foo *f2",
      "qual_name_offset": 5,
      "short_name": "f2",
      "hover": "Foo *f2 = new Foo()",
      "declarations": [],
      "spell": "8:8-8:10|4259594751088586730|3|2",
      "extent": "8:3-8:22|4259594751088586730|3|0",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
