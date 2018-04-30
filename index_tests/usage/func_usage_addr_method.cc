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
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 9376923949268137283,
      "detailed_name": "void user()",
      "qual_name_offset": 5,
      "short_name": "user",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "5:6-5:10|0|1|2",
      "extent": "5:1-7:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [4636142131003982569],
      "uses": [],
      "callees": ["6:18-6:22|18417145003926999463|3|32", "6:18-6:22|18417145003926999463|3|32"]
    }, {
      "usr": 18417145003926999463,
      "detailed_name": "void Foo::Used()",
      "qual_name_offset": 5,
      "short_name": "Used",
      "kind": 6,
      "storage": 1,
      "declarations": ["2:8-2:12|15041163540773201510|2|1"],
      "declaring_type": 15041163540773201510,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:18-6:22|9376923949268137283|3|32"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
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
      "instances": [],
      "uses": ["6:13-6:16|0|1|4"]
    }],
  "usr2var": [{
      "usr": 4636142131003982569,
      "detailed_name": "void (Foo::*)() x",
      "qual_name_offset": 16,
      "short_name": "x",
      "declarations": [],
      "spell": "6:8-6:9|9376923949268137283|3|2",
      "extent": "6:3-6:22|9376923949268137283|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
