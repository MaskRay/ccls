class Foo {
  Foo* make();
};

Foo* Foo::make() {
  Foo f;
  return nullptr;
}

extern Foo foo;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [0, 1],
      "uses": ["2:3-2:6|-1|1|4", "5:1-5:4|-1|1|4", "5:6-5:9|-1|1|4", "6:3-6:6|-1|1|4", "10:8-10:11|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 9488177941273031343,
      "detailed_name": "Foo *Foo::make()",
      "short_name": "make",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "2:8-2:12|0|2|1",
          "param_spellings": []
        }],
      "spell": "5:11-5:15|0|2|2",
      "extent": "5:1-8:2|-1|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [0],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 14873619387499024780,
      "detailed_name": "Foo f",
      "short_name": "f",
      "declarations": [],
      "spell": "6:7-6:8|0|3|2",
      "extent": "6:3-6:8|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 14455976355866885943,
      "detailed_name": "Foo foo",
      "short_name": "foo",
      "declarations": ["10:12-10:15|-1|1|1"],
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 2
    }]
}
*/
