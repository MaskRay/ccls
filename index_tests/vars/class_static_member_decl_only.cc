class Foo {
  static int member;
};
/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "id": 1,
      "usr": 17,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": []
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 5844987037615239736,
      "detailed_name": "int Foo::member",
      "qual_name_offset": 4,
      "short_name": "member",
      "declarations": ["2:14-2:20|0|2|1"],
      "type": 1,
      "uses": [],
      "kind": 8,
      "storage": 3
    }]
}
*/
