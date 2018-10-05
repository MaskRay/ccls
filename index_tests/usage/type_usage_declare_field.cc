struct ForwardType;
struct ImplementedType {};

struct Foo {
  ForwardType* a;
  ImplementedType b;
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 8508299082070213750,
      "detailed_name": "struct ImplementedType {}",
      "qual_name_offset": 7,
      "short_name": "ImplementedType",
      "spell": "2:8-2:23|2:1-2:26|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [14727441168849658842],
      "uses": ["6:3-6:18|4|-1"]
    }, {
      "usr": 13749354388332789217,
      "detailed_name": "struct ForwardType",
      "qual_name_offset": 7,
      "short_name": "ForwardType",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": ["1:8-1:19|1:1-1:19|1|-1"],
      "derived": [],
      "instances": [14314859014962085433],
      "uses": ["5:3-5:14|4|-1"]
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "4:8-4:11|4:1-7:2|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [{
          "L": 14314859014962085433,
          "R": 0
        }, {
          "L": 14727441168849658842,
          "R": 64
        }],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 14314859014962085433,
      "detailed_name": "ForwardType *Foo::a",
      "qual_name_offset": 13,
      "short_name": "a",
      "spell": "5:16-5:17|5:3-5:17|1026|-1",
      "type": 13749354388332789217,
      "kind": 8,
      "parent_kind": 23,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 14727441168849658842,
      "detailed_name": "ImplementedType Foo::b",
      "qual_name_offset": 16,
      "short_name": "b",
      "spell": "6:19-6:20|6:3-6:20|1026|-1",
      "type": 8508299082070213750,
      "kind": 8,
      "parent_kind": 23,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
