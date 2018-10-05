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
      "spell": "1:12-1:18|1:1-3:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["6:11-6:17|36|-1"]
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [4220150017963593039],
      "uses": []
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "spell": "5:7-5:10|5:1-7:2|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [{
          "L": 4220150017963593039,
          "R": 0
        }],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 4220150017963593039,
      "detailed_name": "int Foo::x",
      "qual_name_offset": 4,
      "short_name": "x",
      "hover": "int Foo::x = helper()",
      "spell": "6:7-6:8|6:3-6:19|1026|-1",
      "type": 53,
      "kind": 8,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
