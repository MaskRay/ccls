struct Foo {
  void foo();
};

void usage() {
  Foo* f = nullptr;
  f->foo();
}
/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 6767773193109753523,
      "detailed_name": "void usage()",
      "qual_name_offset": 5,
      "short_name": "usage",
      "spell": "5:6-5:11|5:1-8:2|2|-1",
      "bases": [],
      "vars": [16229832321010999607],
      "callees": ["7:6-7:9|17922201480358737771|3|16420"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 17922201480358737771,
      "detailed_name": "void Foo::foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["2:8-2:11|2:3-2:13|1025|-1"],
      "derived": [],
      "uses": ["7:6-7:9|16420|-1"]
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "1:8-1:11|1:1-3:2|2|-1",
      "bases": [],
      "funcs": [17922201480358737771],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [16229832321010999607],
      "uses": ["6:3-6:6|4|-1"]
    }],
  "usr2var": [{
      "usr": 16229832321010999607,
      "detailed_name": "Foo *f",
      "qual_name_offset": 5,
      "short_name": "f",
      "hover": "Foo *f = nullptr",
      "spell": "6:8-6:9|6:3-6:19|2|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["7:3-7:4|12|-1"]
    }]
}
*/
