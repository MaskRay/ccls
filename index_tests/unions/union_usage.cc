union Foo {
  int a : 5;
  bool b : 3;
};

Foo f;

void act(Foo*) {
  f.a = 3;
}

/*
// TODO: instantiations on Foo should include parameter?

OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 13982179977217945200,
      "detailed_name": "void act(Foo *)",
      "qual_name_offset": 5,
      "short_name": "act",
      "spell": "8:6-8:9|8:1-10:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 37,
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
      "instances": [8804696910588009104],
      "uses": []
    }, {
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
      "instances": [9529311430721959843],
      "uses": []
    }, {
      "usr": 8501689086387244262,
      "detailed_name": "union Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "spell": "1:7-1:10|1:1-4:2|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [{
          "L": 9529311430721959843,
          "R": 0
        }, {
          "L": 8804696910588009104,
          "R": 0
        }],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [2933643612409209903],
      "uses": ["6:1-6:4|4|-1", "8:10-8:13|4|-1"]
    }],
  "usr2var": [{
      "usr": 2933643612409209903,
      "detailed_name": "Foo f",
      "qual_name_offset": 4,
      "short_name": "f",
      "spell": "6:5-6:6|6:1-6:6|2|-1",
      "type": 8501689086387244262,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": ["9:3-9:4|4|-1"]
    }, {
      "usr": 8804696910588009104,
      "detailed_name": "bool Foo::b : 3",
      "qual_name_offset": 5,
      "short_name": "b",
      "spell": "3:8-3:9|3:3-3:13|1026|-1",
      "type": 37,
      "kind": 8,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 9529311430721959843,
      "detailed_name": "int Foo::a : 5",
      "qual_name_offset": 4,
      "short_name": "a",
      "spell": "2:7-2:8|2:3-2:12|1026|-1",
      "type": 53,
      "kind": 8,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "uses": ["9:5-9:6|20|-1"]
    }]
}
*/
