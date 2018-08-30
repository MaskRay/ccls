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
      "kind": 10,
      "declarations": [],
      "spell": "1:6-1:7|0|1|2",
      "extent": "1:1-1:10|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["9:5-9:6|0|1|4"]
    }, {
      "usr": 7074603899792463171,
      "detailed_name": "Inner",
      "qual_name_offset": 0,
      "short_name": "Inner",
      "kind": 26,
      "declarations": [],
      "spell": "6:10-6:15|0|1|2",
      "extent": "6:3-6:18|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [16721564935990383768],
      "uses": []
    }, {
      "usr": 10528472276654770367,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "5:8-5:11|0|1|2",
      "extent": "5:1-7:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [13938528237873543349],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["9:1-9:4|0|1|4", "10:1-10:4|0|1|4"]
    }, {
      "usr": 11976530632376795217,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 26,
      "declarations": [],
      "spell": "5:8-5:11|0|1|2",
      "extent": "4:1-7:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [12028309045033782423],
      "uses": []
    }, {
      "usr": 13892793056005362145,
      "detailed_name": "enum B {}",
      "qual_name_offset": 5,
      "short_name": "B",
      "kind": 10,
      "declarations": [],
      "spell": "2:6-2:7|0|1|2",
      "extent": "2:1-2:10|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["10:5-10:6|0|1|4"]
    }, {
      "usr": 13938528237873543349,
      "detailed_name": "struct Foo::Inner {}",
      "qual_name_offset": 7,
      "short_name": "Inner",
      "kind": 23,
      "declarations": [],
      "spell": "6:10-6:15|10528472276654770367|2|1026",
      "extent": "6:3-6:18|10528472276654770367|2|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["9:9-9:14|0|1|4"]
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "Foo<B> b",
      "qual_name_offset": 7,
      "short_name": "b",
      "declarations": [],
      "spell": "10:8-10:9|0|1|2",
      "extent": "10:1-10:9|0|1|0",
      "type": 11976530632376795217,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "Foo<A>::Inner a",
      "qual_name_offset": 14,
      "short_name": "a",
      "declarations": [],
      "spell": "9:15-9:16|0|1|2",
      "extent": "9:1-9:16|0|1|0",
      "type": 7074603899792463171,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
