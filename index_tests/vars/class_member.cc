class Foo {
  Foo* member;
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
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0],
      "instances": [0],
      "uses": ["2:3-2:6|-1|1|4"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 13799811842374292251,
      "detailed_name": "Foo *Foo::member",
      "short_name": "member",
      "declarations": [],
      "spell": "2:8-2:14|0|2|2",
      "extent": "2:3-2:14|0|2|0",
      "type": 0,
      "uses": [],
      "kind": 8,
      "storage": 0
    }]
}
*/
