class Foo {
  void declonly();
  virtual void purevirtual() = 0;
  void def();
};

void Foo::def() {}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 4012226004228259562,
      "detailed_name": "void Foo::declonly()",
      "qual_name_offset": 5,
      "short_name": "declonly",
      "kind": 6,
      "storage": 0,
      "declarations": ["2:8-2:16|15041163540773201510|2|1"],
      "declaring_type": 15041163540773201510,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 10939323144126021546,
      "detailed_name": "virtual void Foo::purevirtual() = 0",
      "qual_name_offset": 13,
      "short_name": "purevirtual",
      "kind": 6,
      "storage": 0,
      "declarations": [],
      "spell": "3:16-3:27|15041163540773201510|2|2",
      "extent": "3:3-3:33|15041163540773201510|2|0",
      "declaring_type": 15041163540773201510,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 15416083548883122431,
      "detailed_name": "void Foo::def()",
      "qual_name_offset": 5,
      "short_name": "def",
      "kind": 6,
      "storage": 0,
      "declarations": ["4:8-4:11|15041163540773201510|2|1"],
      "spell": "7:11-7:14|15041163540773201510|2|2",
      "extent": "7:1-7:19|0|1|0",
      "declaring_type": 15041163540773201510,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|0|1|2",
      "extent": "1:1-5:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [4012226004228259562, 10939323144126021546, 15416083548883122431],
      "vars": [],
      "instances": [],
      "uses": ["7:6-7:9|0|1|4"]
    }],
  "usr2var": []
}
*/
