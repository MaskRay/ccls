struct Foo {
  void Used();
};

void user() {
  auto x = &Foo::Used;
}


/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 9376923949268137283,
      "detailed_name": "void user()",
      "qual_name_offset": 5,
      "short_name": "user",
      "spell": "5:6-5:10|5:1-7:2|2|-1",
      "bases": [],
      "vars": [4636142131003982569],
      "callees": ["6:18-6:22|18417145003926999463|3|132", "6:18-6:22|18417145003926999463|3|132"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 18417145003926999463,
      "detailed_name": "void Foo::Used()",
      "qual_name_offset": 5,
      "short_name": "Used",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["2:8-2:12|2:3-2:14|1025|-1"],
      "derived": [],
      "uses": ["6:18-6:22|132|-1"]
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "1:8-1:11|1:1-3:2|2|-1",
      "bases": [],
      "funcs": [18417145003926999463],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["6:13-6:16|4|-1"]
    }],
  "usr2var": [{
      "usr": 4636142131003982569,
      "detailed_name": "void (Foo::*)() x",
      "qual_name_offset": 16,
      "short_name": "x",
      "hover": "void (Foo::*)() x = &Foo::Used",
      "spell": "6:8-6:9|6:3-6:22|2|-1",
      "type": 0,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
