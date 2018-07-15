struct Foo {
  static int x;
};

void accept(int);

void foo() {
  accept(Foo::x);
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
      "spell": "7:6-7:9|0|1|2",
      "extent": "7:1-9:2|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 17175780305784503374,
      "detailed_name": "void accept(int)",
      "qual_name_offset": 5,
      "short_name": "accept",
      "kind": 12,
      "storage": 0,
      "declarations": ["5:6-5:12|0|1|1"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["8:3-8:9|0|1|16420"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [8599782646965457351],
      "uses": []
    }, {
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
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["8:10-8:13|0|1|4"]
    }],
  "usr2var": [{
      "usr": 8599782646965457351,
      "detailed_name": "static int Foo::x",
      "qual_name_offset": 11,
      "short_name": "x",
      "declarations": ["2:14-2:15|15041163540773201510|2|1025"],
      "type": 53,
      "uses": ["8:15-8:16|15041163540773201510|2|12"],
      "kind": 13,
      "storage": 2
    }]
}
*/
