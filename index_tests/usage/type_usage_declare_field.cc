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
  "types": [{
      "id": 0,
      "usr": 13749354388332789217,
      "detailed_name": "ForwardType",
      "short_name": "ForwardType",
      "kind": 23,
      "declarations": ["1:8-1:19|-1|1|1"],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["5:3-5:14|-1|1|4"]
    }, {
      "id": 1,
      "usr": 8508299082070213750,
      "detailed_name": "ImplementedType",
      "short_name": "ImplementedType",
      "kind": 23,
      "declarations": [],
      "spell": "2:8-2:23|-1|1|2",
      "extent": "2:1-2:26|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [1],
      "uses": ["6:3-6:18|-1|1|4"]
    }, {
      "id": 2,
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "4:8-4:11|-1|1|2",
      "extent": "4:1-7:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0, 1],
      "instances": [],
      "uses": []
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 14314859014962085433,
      "detailed_name": "ForwardType *Foo::a",
      "short_name": "a",
      "declarations": [],
      "spell": "5:16-5:17|2|2|2",
      "extent": "5:3-5:17|2|2|0",
      "type": 0,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "id": 1,
      "usr": 14727441168849658842,
      "detailed_name": "ImplementedType Foo::b",
      "short_name": "b",
      "declarations": [],
      "spell": "6:19-6:20|2|2|2",
      "extent": "6:3-6:20|2|2|0",
      "type": 1,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
