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
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "1:6-1:12|0|1|2",
      "extent": "1:1-1:17|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["8:3-8:9|0|1|8228"],
      "callees": []
    }, {
      "usr": 3385168158331140247,
      "detailed_name": "Foo::Foo()",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 9,
      "storage": 0,
      "declarations": ["4:3-4:6|15041163540773201510|2|513"],
      "spell": "7:6-7:9|15041163540773201510|2|514",
      "extent": "4:3-4:8|15041163540773201510|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "3:8-3:11|0|1|2",
      "extent": "3:1-5:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [3385168158331140247],
      "vars": [],
      "instances": [],
      "uses": ["4:3-4:6|0|1|4", "7:1-7:4|0|1|4", "7:6-7:9|0|1|4"]
    }],
  "usr2var": []
}
*/
