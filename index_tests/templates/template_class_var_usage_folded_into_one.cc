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
      "instances": [13545144895171991916, 16721564935990383768, 12028309045033782423],
      "uses": []
    }, {
      "usr": 10528472276654770367,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "2:8-2:11|0|1|2",
      "extent": "2:1-4:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["6:9-6:12|0|1|4", "7:9-7:12|0|1|4"]
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int b = Foo<bool>::var",
      "declarations": [],
      "spell": "7:5-7:6|0|1|2",
      "extent": "7:1-7:23|0|1|0",
      "type": 53,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 13545144895171991916,
      "detailed_name": "static constexpr int Foo::var",
      "qual_name_offset": 21,
      "short_name": "var",
      "hover": "static constexpr int Foo::var = 3",
      "declarations": ["3:24-3:27|10528472276654770367|2|513"],
      "type": 53,
      "uses": ["6:19-6:22|10528472276654770367|2|12", "7:20-7:23|10528472276654770367|2|12"],
      "kind": 13,
      "storage": 2
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "hover": "int a = Foo<int>::var",
      "declarations": [],
      "spell": "6:5-6:6|0|1|2",
      "extent": "6:1-6:22|0|1|0",
      "type": 53,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
