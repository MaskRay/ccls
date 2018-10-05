class Foo {
  void foo() const;
};

void Foo::foo() const {}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 6446764306530590711,
      "detailed_name": "void Foo::foo() const",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "5:11-5:14|5:1-5:25|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 5,
      "storage": 0,
      "declarations": ["2:8-2:11|2:3-2:19|1025|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "spell": "1:7-1:10|1:1-3:2|2|-1",
      "bases": [],
      "funcs": [6446764306530590711],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["5:6-5:9|4|-1"]
    }],
  "usr2var": []
}
*/
