class Foo {
  static int member;
};
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
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [5844987037615239736],
      "uses": []
    }, {
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
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 5844987037615239736,
      "detailed_name": "static int Foo::member",
      "qual_name_offset": 11,
      "short_name": "member",
      "type": 53,
      "kind": 13,
      "parent_kind": 5,
      "storage": 2,
      "declarations": ["2:14-2:20|2:3-2:20|1025|-1"],
      "uses": []
    }]
}
*/
