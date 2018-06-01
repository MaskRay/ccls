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
  "skipped_by_preprocessor": [],
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
      "uses": ["8:3-8:9|3385168158331140247|3|32"],
      "callees": []
    }, {
      "usr": 3385168158331140247,
      "detailed_name": "Foo::Foo()",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 9,
      "storage": 0,
      "declarations": ["4:3-4:6|15041163540773201510|2|1"],
      "spell": "7:6-7:9|15041163540773201510|2|2",
      "extent": "7:1-9:2|0|1|0",
      "declaring_type": 15041163540773201510,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["8:3-8:9|468307235068920063|3|32"]
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 23,
      "declarations": ["4:3-4:6|0|1|4", "7:6-7:9|0|1|4"],
      "spell": "3:8-3:11|0|1|2",
      "extent": "3:1-5:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [3385168158331140247],
      "vars": [],
      "instances": [],
      "uses": ["4:3-4:6|15041163540773201510|2|4", "7:6-7:9|0|1|4", "7:1-7:4|0|1|4"]
    }],
  "usr2var": []
}
*/
