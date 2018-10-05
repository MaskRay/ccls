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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 6666242542855173890,
      "detailed_name": "void Derived::foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "5:8-5:11|5:3-5:25|5186|-1",
      "bases": [9948027785633571339],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 9948027785633571339,
      "detailed_name": "virtual void Root::foo()",
      "qual_name_offset": 13,
      "short_name": "foo",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["2:16-2:19|2:3-2:21|1089|-1"],
      "derived": [6666242542855173890],
      "uses": []
    }],
  "usr2type": [{
      "usr": 3897841498936210886,
      "detailed_name": "class Root {}",
      "qual_name_offset": 6,
      "short_name": "Root",
      "spell": "1:7-1:11|1:1-3:2|2|-1",
      "bases": [],
      "funcs": [9948027785633571339],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [10963370434658308541],
      "instances": [],
      "uses": ["4:24-4:28|2052|-1"]
    }, {
      "usr": 10963370434658308541,
      "detailed_name": "class Derived : public Root {}",
      "qual_name_offset": 6,
      "short_name": "Derived",
      "spell": "4:7-4:14|4:1-6:2|2|-1",
      "bases": [3897841498936210886],
      "funcs": [6666242542855173890],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
