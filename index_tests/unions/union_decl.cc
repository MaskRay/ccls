union Foo {
  int a;
  bool b;
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
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
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 8804696910588009104,
      "detailed_name": "bool Foo::b",
      "qual_name_offset": 5,
      "short_name": "b",
      "spell": "3:8-3:9|3:3-3:9|1026|-1",
      "type": 37,
      "kind": 8,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 9529311430721959843,
      "detailed_name": "int Foo::a",
      "qual_name_offset": 4,
      "short_name": "a",
      "spell": "2:7-2:8|2:3-2:8|1026|-1",
      "type": 53,
      "kind": 8,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
