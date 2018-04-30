class Foo {
  static Foo* member;
};
Foo* Foo::member = nullptr;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
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
      "vars": [5844987037615239736],
      "instances": [5844987037615239736],
      "uses": ["2:10-2:13|0|1|4", "4:1-4:4|0|1|4", "4:6-4:9|0|1|4"]
    }],
  "usr2var": [{
      "usr": 5844987037615239736,
      "detailed_name": "Foo *Foo::member",
      "qual_name_offset": 5,
      "short_name": "member",
      "hover": "Foo *Foo::member = nullptr",
      "declarations": ["2:15-2:21|15041163540773201510|2|1"],
      "spell": "4:11-4:17|15041163540773201510|2|2",
      "extent": "4:1-4:27|0|1|0",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 8,
      "storage": 1
    }]
}
*/
