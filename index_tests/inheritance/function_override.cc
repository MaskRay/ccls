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
      "kind": 6,
      "storage": 0,
      "declarations": [],
      "spell": "5:8-5:11|10963370434658308541|2|2626",
      "extent": "5:3-5:25|10963370434658308541|2|0",
      "declaring_type": 0,
      "bases": [9948027785633571339],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 9948027785633571339,
      "detailed_name": "virtual void Root::foo()",
      "qual_name_offset": 13,
      "short_name": "foo",
      "kind": 6,
      "storage": 0,
      "declarations": ["2:16-2:19|3897841498936210886|2|577"],
      "declaring_type": 0,
      "bases": [],
      "derived": [6666242542855173890],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 3897841498936210886,
      "detailed_name": "class Root {}",
      "qual_name_offset": 6,
      "short_name": "Root",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:11|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [10963370434658308541],
      "types": [],
      "funcs": [9948027785633571339],
      "vars": [],
      "instances": [],
      "uses": ["4:24-4:28|0|1|1028"]
    }, {
      "usr": 10963370434658308541,
      "detailed_name": "class Derived : public Root {}",
      "qual_name_offset": 6,
      "short_name": "Derived",
      "kind": 5,
      "declarations": [],
      "spell": "4:7-4:14|0|1|2",
      "extent": "4:1-6:2|0|1|0",
      "alias_of": 0,
      "bases": [3897841498936210886],
      "derived": [],
      "types": [],
      "funcs": [6666242542855173890],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
