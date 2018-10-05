enum A {};
enum B {};

template<typename T>
struct Foo {
  struct Inner {};
};

Foo<A>::Inner a;
Foo<B> b;
/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 6697181287623958829,
      "detailed_name": "enum A {}",
      "qual_name_offset": 5,
      "short_name": "A",
      "spell": "1:6-1:7|1:1-1:10|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 10,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["9:5-9:6|4|-1"]
    }, {
      "usr": 10528472276654770367,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "5:8-5:11|5:1-7:2|2|-1",
      "bases": [],
      "funcs": [],
      "types": [13938528237873543349],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [12028309045033782423],
      "uses": ["9:1-9:4|4|-1", "10:1-10:4|4|-1"]
    }, {
      "usr": 13892793056005362145,
      "detailed_name": "enum B {}",
      "qual_name_offset": 5,
      "short_name": "B",
      "spell": "2:6-2:7|2:1-2:10|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 10,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["10:5-10:6|4|-1"]
    }, {
      "usr": 13938528237873543349,
      "detailed_name": "struct Foo::Inner {}",
      "qual_name_offset": 7,
      "short_name": "Inner",
      "spell": "6:10-6:15|6:3-6:18|1026|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 23,
      "declarations": [],
      "derived": [],
      "instances": [16721564935990383768],
      "uses": ["9:9-9:14|4|-1"]
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "Foo<B> b",
      "qual_name_offset": 7,
      "short_name": "b",
      "spell": "10:8-10:9|10:1-10:9|2|-1",
      "type": 10528472276654770367,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "Foo<A>::Inner a",
      "qual_name_offset": 14,
      "short_name": "a",
      "spell": "9:15-9:16|9:1-9:16|2|-1",
      "type": 13938528237873543349,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
