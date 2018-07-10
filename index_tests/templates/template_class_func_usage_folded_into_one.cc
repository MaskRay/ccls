template<typename T>
struct Foo {
  static int foo() {
    return 3;
  }
};

int a = Foo<int>::foo();
int b = Foo<bool>::foo();

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 8340731781048851399,
      "detailed_name": "static int Foo::foo()",
      "qual_name_offset": 11,
      "short_name": "foo",
      "kind": 6,
      "storage": 0,
      "declarations": [],
      "spell": "3:14-3:17|10528472276654770367|2|1026",
      "extent": "3:3-5:4|10528472276654770367|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["8:19-8:22|10528472276654770367|2|36", "9:20-9:23|10528472276654770367|2|36"],
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
      "instances": [16721564935990383768, 12028309045033782423],
      "uses": []
    }, {
      "usr": 10528472276654770367,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "2:8-2:11|0|1|2",
      "extent": "2:1-6:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [8340731781048851399],
      "vars": [],
      "instances": [],
      "uses": ["8:9-8:12|0|1|4", "9:9-9:12|0|1|4"]
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int b = Foo<bool>::foo()",
      "declarations": [],
      "spell": "9:5-9:6|0|1|2",
      "extent": "9:1-9:25|0|1|0",
      "type": 53,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "hover": "int a = Foo<int>::foo()",
      "declarations": [],
      "spell": "8:5-8:6|0|1|2",
      "extent": "8:1-8:24|0|1|0",
      "type": 53,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
