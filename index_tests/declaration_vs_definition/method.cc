class Foo {
  void declonly();
  virtual void purevirtual() = 0;
  void def();
};

void Foo::def() {}

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
      "extent": "1:1-5:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0, 1, 2],
      "vars": [],
      "instances": [],
      "uses": ["7:6-7:9|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 4012226004228259562,
      "detailed_name": "void Foo::declonly()",
      "short_name": "declonly",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "2:8-2:16|0|2|1",
          "param_spellings": []
        }],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 1,
      "usr": 10939323144126021546,
      "detailed_name": "void Foo::purevirtual()",
      "short_name": "purevirtual",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "3:16-3:27|0|2|1",
          "param_spellings": []
        }],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 2,
      "usr": 15416083548883122431,
      "detailed_name": "void Foo::def()",
      "short_name": "def",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "4:8-4:11|0|2|1",
          "param_spellings": []
        }],
      "spell": "7:11-7:14|0|2|2",
      "extent": "7:1-7:19|-1|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
*/
