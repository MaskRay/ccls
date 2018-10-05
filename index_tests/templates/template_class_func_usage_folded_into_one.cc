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
      "spell": "3:14-3:17|3:3-5:4|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 254,
      "parent_kind": 23,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["8:19-8:22|36|-1", "9:20-9:23|36|-1"]
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [16721564935990383768, 12028309045033782423],
      "uses": []
    }, {
      "usr": 10528472276654770367,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "2:8-2:11|2:1-6:2|2|-1",
      "bases": [],
      "funcs": [8340731781048851399],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["8:9-8:12|4|-1", "9:9-9:12|4|-1"]
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int b = Foo<bool>::foo()",
      "spell": "9:5-9:6|9:1-9:25|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "hover": "int a = Foo<int>::foo()",
      "spell": "8:5-8:6|8:1-8:24|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
