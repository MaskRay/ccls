union Foo {
  int a;
  bool b;
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 8501689086387244262,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "1:7-1:10|-1|1|2",
      "extent": "1:1-4:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0, 1],
      "instances": [],
      "uses": []
    }, {
      "id": 1,
      "usr": 17,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": []
    }, {
      "id": 2,
      "usr": 3,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [1],
      "uses": []
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 9529311430721959843,
      "detailed_name": "int Foo::a",
      "short_name": "a",
      "declarations": [],
      "spell": "2:7-2:8|0|2|2",
      "extent": "2:3-2:8|0|2|0",
      "type": 1,
      "uses": [],
      "kind": 8,
      "storage": 0
    }, {
      "id": 1,
      "usr": 8804696910588009104,
      "detailed_name": "bool Foo::b",
      "short_name": "b",
      "declarations": [],
      "spell": "3:8-3:9|0|2|2",
      "extent": "3:3-3:9|0|2|0",
      "type": 2,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
