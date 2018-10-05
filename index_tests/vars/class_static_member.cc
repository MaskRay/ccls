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
      "spell": "1:7-1:10|1:1-3:2|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [5844987037615239736, 5844987037615239736],
      "uses": ["2:10-2:13|4|-1", "4:1-4:4|4|-1", "4:6-4:9|4|-1"]
    }],
  "usr2var": [{
      "usr": 5844987037615239736,
      "detailed_name": "static Foo *Foo::member",
      "qual_name_offset": 12,
      "short_name": "member",
      "spell": "4:11-4:17|4:1-4:27|1026|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 5,
      "storage": 2,
      "declarations": ["2:15-2:21|2:3-2:21|1025|-1"],
      "uses": []
    }]
}
*/
