struct Foo {
  void Used();
};

void user() {
  Foo* f = nullptr;
  f->Used();
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
      "spell": "5:6-5:10|5:1-8:2|2|-1",
      "bases": [],
      "vars": [14045150712868309451],
      "callees": ["7:6-7:10|18417145003926999463|3|16420"],
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
      "uses": ["7:6-7:10|16420|-1"]
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
      "instances": [14045150712868309451],
      "uses": ["6:3-6:6|4|-1"]
    }],
  "usr2var": [{
      "usr": 14045150712868309451,
      "detailed_name": "Foo *f",
      "qual_name_offset": 5,
      "short_name": "f",
      "hover": "Foo *f = nullptr",
      "spell": "6:8-6:9|6:3-6:19|2|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": ["7:3-7:4|12|-1"]
    }]
}
*/
