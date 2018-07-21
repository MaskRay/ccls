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
      "kind": 6,
      "storage": 0,
      "declarations": ["2:8-2:11|15041163540773201510|2|1025"],
      "spell": "5:11-5:14|15041163540773201510|2|1026",
      "extent": "5:1-5:25|15041163540773201510|2|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [6446764306530590711],
      "vars": [],
      "instances": [],
      "uses": ["5:6-5:9|0|1|4"]
    }],
  "usr2var": []
}
*/
