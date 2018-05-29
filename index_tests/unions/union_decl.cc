union Foo {
  int a;
  bool b;
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 3,
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
      "usr": 17,
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
      "detailed_name": "Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 23,
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
      "detailed_name": "bool Foo::b",
      "qual_name_offset": 5,
      "short_name": "b",
      "declarations": [],
      "spell": "3:8-3:9|8501689086387244262|2|2",
      "extent": "3:3-3:9|8501689086387244262|2|0",
      "type": 3,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "usr": 9529311430721959843,
      "detailed_name": "int Foo::a",
      "qual_name_offset": 4,
      "short_name": "a",
      "declarations": [],
      "spell": "2:7-2:8|8501689086387244262|2|2",
      "extent": "2:3-2:8|8501689086387244262|2|0",
      "type": 17,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
