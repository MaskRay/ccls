static int helper() {
  return 5;
}

class Foo {
  int x = helper();
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 9630503130605430498,
      "detailed_name": "static int helper()",
      "qual_name_offset": 11,
      "short_name": "helper",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "1:12-1:18|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:11-6:17|15041163540773201510|2|36"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [4220150017963593039],
      "uses": []
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "5:7-5:10|0|1|2",
      "extent": "5:1-7:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [{
          "L": 4220150017963593039,
          "R": 0
        }],
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 4220150017963593039,
      "detailed_name": "int Foo::x",
      "qual_name_offset": 4,
      "short_name": "x",
      "hover": "int Foo::x = helper()",
      "declarations": [],
      "spell": "6:7-6:8|15041163540773201510|2|1026",
      "extent": "6:3-6:19|15041163540773201510|2|0",
      "type": 53,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
