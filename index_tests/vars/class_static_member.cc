class Foo {
  static Foo* member;
};
Foo* Foo::member = nullptr;

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
      "uses": ["2:10-2:13|-1|1|4", "4:1-4:4|-1|1|4", "4:6-4:9|-1|1|4"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 5844987037615239736,
      "detailed_name": "Foo *Foo::member",
      "short_name": "member",
      "hover": "Foo *Foo::member = nullptr",
      "declarations": ["2:15-2:21|0|2|1"],
      "spell": "4:11-4:17|0|2|2",
      "extent": "4:1-4:27|-1|1|0",
      "type": 0,
      "uses": [],
      "kind": 8,
      "storage": 1
    }]
}
*/
