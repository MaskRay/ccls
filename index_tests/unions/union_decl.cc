union Foo {
  int a;
  bool b;
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 8501689086387244262,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [9529311430721959843, 8804696910588009104],
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
      "spell": "1:7-1:10|0|1|2",
      "extent": "1:1-4:2|0|1|0",
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
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 8804696910588009104,
      "detailed_name": "Foo::bool b",
      "qual_name_offset": 0,
      "short_name": "b",
      "declarations": [],
      "spell": "3:8-3:9|8501689086387244262|2|514",
      "extent": "3:3-3:9|8501689086387244262|2|0",
      "type": 37,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "usr": 9529311430721959843,
      "detailed_name": "int Foo::a",
      "qual_name_offset": 4,
      "short_name": "a",
      "declarations": [],
      "spell": "2:7-2:8|8501689086387244262|2|514",
      "extent": "2:3-2:8|8501689086387244262|2|0",
      "type": 53,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
