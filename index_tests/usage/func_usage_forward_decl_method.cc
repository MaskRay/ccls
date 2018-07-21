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
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "5:6-5:11|0|1|2",
      "extent": "5:1-8:2|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [16229832321010999607],
      "uses": [],
      "callees": ["7:6-7:9|17922201480358737771|3|16420"]
    }, {
      "usr": 17922201480358737771,
      "detailed_name": "void Foo::foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 6,
      "storage": 0,
      "declarations": ["2:8-2:11|15041163540773201510|2|1025"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["7:6-7:9|6767773193109753523|3|16420"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:11|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [17922201480358737771],
      "vars": [],
      "instances": [16229832321010999607],
      "uses": ["6:3-6:6|6767773193109753523|3|4"]
    }],
  "usr2var": [{
      "usr": 16229832321010999607,
      "detailed_name": "Foo *f",
      "qual_name_offset": 5,
      "short_name": "f",
      "hover": "Foo *f = nullptr",
      "declarations": [],
      "spell": "6:8-6:9|6767773193109753523|3|2",
      "extent": "6:3-6:19|6767773193109753523|3|0",
      "type": 15041163540773201510,
      "uses": ["7:3-7:4|6767773193109753523|3|12"],
      "kind": 13,
      "storage": 0
    }]
}
*/
