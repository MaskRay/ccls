template<typename T>
struct Foo {
  template<typename R>
  static int foo() {
    return 3;
  }
};

int a = Foo<int>::foo<float>();
int b = Foo<bool>::foo<double>();

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 9034026360701857235,
      "detailed_name": "static int Foo::foo()",
      "qual_name_offset": 11,
      "short_name": "foo",
      "spell": "4:14-4:17|4:3-6:4|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 254,
      "parent_kind": 23,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["9:19-9:22|36|-1", "10:20-10:23|36|-1"]
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
      "spell": "2:8-2:11|2:1-7:2|2|-1",
      "bases": [],
      "funcs": [9034026360701857235],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["9:9-9:12|4|-1", "10:9-10:12|4|-1"]
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int b = Foo<bool>::foo<double>()",
      "spell": "10:5-10:6|10:1-10:33|2|-1",
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
      "hover": "int a = Foo<int>::foo<float>()",
      "spell": "9:5-9:6|9:1-9:31|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
