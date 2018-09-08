class Foo {
  void foo() {}
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 17922201480358737771,
      "detailed_name": "void Foo::foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 6,
      "storage": 0,
      "declarations": [],
      "spell": "2:8-2:11|15041163540773201510|2|1026|-1",
      "extent": "2:3-2:16|15041163540773201510|2|0|-1",
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
      "spell": "1:7-1:10|0|1|2|-1",
      "extent": "1:1-3:2|0|1|0|-1",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [17922201480358737771],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
