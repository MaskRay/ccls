class Foo {
  Foo* member;
};
/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [{
          "L": 13799811842374292251,
          "R": 0
        }],
      "instances": [13799811842374292251],
      "uses": ["2:3-2:6|0|1|4"]
    }],
  "usr2var": [{
      "usr": 13799811842374292251,
      "detailed_name": "Foo *Foo::member",
      "qual_name_offset": 5,
      "short_name": "member",
      "declarations": [],
      "spell": "2:8-2:14|15041163540773201510|2|1026",
      "extent": "2:3-2:14|15041163540773201510|2|0",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
