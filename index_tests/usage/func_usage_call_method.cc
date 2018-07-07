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
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "5:6-5:10|0|1|2",
      "extent": "5:1-8:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 18417145003926999463,
      "detailed_name": "void Foo::Used()",
      "qual_name_offset": 5,
      "short_name": "Used",
      "kind": 6,
      "storage": 0,
      "declarations": ["2:8-2:12|15041163540773201510|2|513"],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["7:6-7:10|15041163540773201510|2|8228"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:11|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [18417145003926999463],
      "vars": [],
      "instances": [14045150712868309451],
      "uses": ["6:3-6:6|0|1|4"]
    }],
  "usr2var": [{
      "usr": 14045150712868309451,
      "detailed_name": "Foo *f",
      "qual_name_offset": 5,
      "short_name": "f",
      "hover": "Foo *f = nullptr",
      "declarations": [],
      "spell": "6:8-6:9|9376923949268137283|3|2",
      "extent": "6:3-6:19|9376923949268137283|3|0",
      "type": 15041163540773201510,
      "uses": ["7:3-7:4|9376923949268137283|3|12"],
      "kind": 13,
      "storage": 0
    }]
}
*/
