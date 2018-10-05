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
      "spell": "2:7-2:10|2:1-2:13|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [16721564935990383768, 12028309045033782423],
      "uses": ["4:1-4:4|4|-1", "5:1-5:4|4|-1"]
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "Foo<bool> b",
      "qual_name_offset": 10,
      "short_name": "b",
      "spell": "5:11-5:12|5:1-5:12|2|-1",
      "type": 10528472276654770367,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "Foo<int> a",
      "qual_name_offset": 9,
      "short_name": "a",
      "spell": "4:10-4:11|4:1-4:11|2|-1",
      "type": 10528472276654770367,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
