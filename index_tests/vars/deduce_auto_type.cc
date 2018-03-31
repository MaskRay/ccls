class Foo {};
void f() {
  auto x = new Foo();
  auto* y = new Foo();
}

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
      "extent": "1:1-1:13|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1],
      "uses": ["3:16-3:19|-1|1|4", "4:17-4:20|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 880549676430489861,
      "detailed_name": "void f()",
      "short_name": "f",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "2:6-2:7|-1|1|2",
      "extent": "2:1-5:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0, 1],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 9275666070987716270,
      "detailed_name": "Foo *x",
      "short_name": "x",
      "declarations": [],
      "spell": "3:8-3:9|0|3|2",
      "extent": "3:3-3:21|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 16202433437488621027,
      "detailed_name": "Foo *y",
      "short_name": "y",
      "declarations": [],
      "spell": "4:9-4:10|0|3|2",
      "extent": "4:3-4:22|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
