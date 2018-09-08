class Foo {
  static Foo* member;
};
Foo* Foo::member = nullptr;

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
      "spell": "1:7-1:10|0|1|2|-1",
      "extent": "1:1-3:2|0|1|0|-1",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [5844987037615239736, 5844987037615239736],
      "uses": ["2:10-2:13|15041163540773201510|2|4|-1", "4:1-4:4|0|1|4|-1", "4:6-4:9|0|1|4|-1"]
    }],
  "usr2var": [{
      "usr": 5844987037615239736,
      "detailed_name": "static Foo *Foo::member",
      "qual_name_offset": 12,
      "short_name": "member",
      "declarations": ["2:15-2:21|2:3-2:21|15041163540773201510|2|1025|-1"],
      "spell": "4:11-4:17|15041163540773201510|2|1026|-1",
      "extent": "4:1-4:27|15041163540773201510|2|0|-1",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 13,
      "storage": 2
    }]
}
*/
