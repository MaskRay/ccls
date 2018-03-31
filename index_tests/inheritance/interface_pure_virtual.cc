class IFoo {
  virtual void foo() = 0;
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 9949214233977131946,
      "detailed_name": "IFoo",
      "short_name": "IFoo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:11|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 3277829753446788562,
      "detailed_name": "void IFoo::foo()",
      "short_name": "foo",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "2:16-2:19|0|2|1",
          "param_spellings": []
        }],
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
