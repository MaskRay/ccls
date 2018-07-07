template<typename T>
class Foo {};

Foo<int> a;
Foo<bool> b;

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 10528472276654770367,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:10|0|1|2",
      "extent": "2:1-2:13|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [16721564935990383768, 12028309045033782423],
      "uses": ["4:1-4:4|0|1|4", "5:1-5:4|0|1|4"]
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "Foo<bool> b",
      "qual_name_offset": 4,
      "short_name": "b",
      "declarations": [],
      "spell": "5:11-5:12|0|1|2",
      "extent": "5:1-5:12|0|1|0",
      "type": 10528472276654770367,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "Foo<int> a",
      "qual_name_offset": 9,
      "short_name": "a",
      "declarations": [],
      "spell": "4:10-4:11|0|1|2",
      "extent": "4:1-4:11|0|1|0",
      "type": 10528472276654770367,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
