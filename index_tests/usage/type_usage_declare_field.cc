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
  "skipped_by_preprocessor": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 8508299082070213750,
      "detailed_name": "ImplementedType",
      "qual_name_offset": 0,
      "short_name": "ImplementedType",
      "kind": 23,
      "declarations": [],
      "spell": "2:8-2:23|0|1|2",
      "extent": "2:1-2:26|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [14727441168849658842],
      "uses": ["6:3-6:18|0|1|4"]
    }, {
      "usr": 13749354388332789217,
      "detailed_name": "ForwardType",
      "qual_name_offset": 0,
      "short_name": "ForwardType",
      "kind": 23,
      "declarations": ["1:8-1:19|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [14314859014962085433],
      "uses": ["5:3-5:14|0|1|4"]
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "4:8-4:11|0|1|2",
      "extent": "4:1-7:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [14314859014962085433, 14727441168849658842],
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 14314859014962085433,
      "detailed_name": "ForwardType *Foo::a",
      "qual_name_offset": 13,
      "short_name": "a",
      "declarations": [],
      "spell": "5:16-5:17|15041163540773201510|2|2",
      "extent": "5:3-5:17|15041163540773201510|2|0",
      "type": 13749354388332789217,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "usr": 14727441168849658842,
      "detailed_name": "ImplementedType Foo::b",
      "qual_name_offset": 16,
      "short_name": "b",
      "declarations": [],
      "spell": "6:19-6:20|15041163540773201510|2|2",
      "extent": "6:3-6:20|15041163540773201510|2|0",
      "type": 8508299082070213750,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
