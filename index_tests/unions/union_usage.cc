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
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "8:6-8:9|0|1|2|-1",
      "extent": "8:1-10:2|0|1|0|-1",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 37,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [8804696910588009104],
      "uses": []
    }, {
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [9529311430721959843],
      "uses": []
    }, {
      "usr": 8501689086387244262,
      "detailed_name": "union Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|0|1|2|-1",
      "extent": "1:1-4:2|0|1|0|-1",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [{
          "L": 9529311430721959843,
          "R": 0
        }, {
          "L": 8804696910588009104,
          "R": 0
        }],
      "instances": [2933643612409209903],
      "uses": ["6:1-6:4|0|1|4|-1", "8:10-8:13|0|1|4|-1"]
    }],
  "usr2var": [{
      "usr": 2933643612409209903,
      "detailed_name": "Foo f",
      "qual_name_offset": 4,
      "short_name": "f",
      "declarations": [],
      "spell": "6:5-6:6|0|1|2|-1",
      "extent": "6:1-6:6|0|1|0|-1",
      "type": 8501689086387244262,
      "uses": ["9:3-9:4|13982179977217945200|3|4|-1"],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 8804696910588009104,
      "detailed_name": "bool Foo::b : 3",
      "qual_name_offset": 5,
      "short_name": "b",
      "declarations": [],
      "spell": "3:8-3:9|8501689086387244262|2|1026|-1",
      "extent": "3:3-3:13|8501689086387244262|2|0|-1",
      "type": 37,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "usr": 9529311430721959843,
      "detailed_name": "int Foo::a : 5",
      "qual_name_offset": 4,
      "short_name": "a",
      "declarations": [],
      "spell": "2:7-2:8|8501689086387244262|2|1026|-1",
      "extent": "2:3-2:12|8501689086387244262|2|0|-1",
      "type": 53,
      "uses": ["9:5-9:6|13982179977217945200|3|20|-1"],
      "kind": 8,
      "storage": 0
    }]
}
*/
