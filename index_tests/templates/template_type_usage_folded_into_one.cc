template<typename T>
class Foo {};

Foo<int> a;
Foo<bool> b;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 10528472276654770367,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:10|-1|1|2",
      "extent": "2:1-2:13|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1],
      "uses": ["4:1-4:4|-1|1|4", "5:1-5:4|-1|1|4"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 16721564935990383768,
      "detailed_name": "Foo<int> a",
      "short_name": "a",
      "declarations": [],
      "spell": "4:10-4:11|-1|1|2",
      "extent": "4:1-4:11|-1|1|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 12028309045033782423,
      "detailed_name": "Foo<bool> b",
      "short_name": "b",
      "declarations": [],
      "spell": "5:11-5:12|-1|1|2",
      "extent": "5:1-5:12|-1|1|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
