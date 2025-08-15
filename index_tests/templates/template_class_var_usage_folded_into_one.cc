template<typename T>
struct Foo {
  static constexpr int var = 3;
};

int a = Foo<int>::var;
int b = Foo<bool>::var;

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 452,
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
      "instances": [13545144895171991916, 16721564935990383768, 12028309045033782423],
      "uses": []
    }, {
      "usr": 8038341777080655976,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 26,
      "parent_kind": 0,
      "declarations": ["1:19-1:20|1:10-1:20|1025|-1"],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 10528472276654770367,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "2:8-2:11|2:1-4:2|2|-1",
      "bases": [],
      "funcs": [],
      "types": [8038341777080655976],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 1,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["6:9-6:12|4|-1", "7:9-7:12|4|-1"]
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int b = Foo<bool>::var",
      "spell": "7:5-7:6|7:1-7:23|2|-1",
      "type": 452,
      "kind": 13,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 13545144895171991916,
      "detailed_name": "static constexpr int Foo::var",
      "qual_name_offset": 21,
      "short_name": "var",
      "hover": "static constexpr int Foo::var = 3",
      "spell": "3:24-3:27|3:3-3:31|1026|-1",
      "type": 452,
      "kind": 8,
      "parent_kind": 23,
      "storage": 2,
      "declarations": [],
      "uses": ["6:19-6:22|12|-1", "7:20-7:23|12|-1"]
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "hover": "int a = Foo<int>::var",
      "spell": "6:5-6:6|6:1-6:22|2|-1",
      "type": 452,
      "kind": 13,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
