class Root {
  virtual void foo();
};
class Derived : public Root {
  void foo() override {}
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 3897841498936210886,
      "detailed_name": "Root",
      "short_name": "Root",
      "kind": 5,
      "declarations": ["4:24-4:28|-1|1|4"],
      "spell": "1:7-1:11|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [],
      "derived": [1],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["4:24-4:28|-1|1|4"]
    }, {
      "id": 1,
      "usr": 10963370434658308541,
      "detailed_name": "Derived",
      "short_name": "Derived",
      "kind": 5,
      "declarations": [],
      "spell": "4:7-4:14|-1|1|2",
      "extent": "4:1-6:2|-1|1|0",
      "bases": [0],
      "derived": [],
      "types": [],
      "funcs": [1],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 9948027785633571339,
      "detailed_name": "void Root::foo()",
      "short_name": "foo",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "2:16-2:19|0|2|1",
          "param_spellings": []
        }],
      "declaring_type": 0,
      "bases": [],
      "derived": [1],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 1,
      "usr": 6666242542855173890,
      "detailed_name": "void Derived::foo()",
      "short_name": "foo",
      "kind": 6,
      "storage": 1,
      "declarations": [],
      "spell": "5:8-5:11|1|2|2",
      "extent": "5:3-5:25|1|2|0",
      "declaring_type": 1,
      "bases": [0],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
*/
