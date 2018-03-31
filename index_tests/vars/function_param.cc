struct Foo;

void foo(Foo* p0, Foo* p1) {}

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
      "kind": 23,
      "declarations": ["1:8-1:11|-1|1|1"],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1],
      "uses": ["3:10-3:13|-1|1|4", "3:19-3:22|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 8908726657907936744,
      "detailed_name": "void foo(Foo *p0, Foo *p1)",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "3:6-3:9|-1|1|2",
      "extent": "3:1-3:30|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0, 1],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 4580260577538694711,
      "detailed_name": "Foo *p0",
      "short_name": "p0",
      "declarations": [],
      "spell": "3:15-3:17|0|3|2",
      "extent": "3:10-3:17|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 253,
      "storage": 1
    }, {
      "id": 1,
      "usr": 12071725611268840435,
      "detailed_name": "Foo *p1",
      "short_name": "p1",
      "declarations": [],
      "spell": "3:24-3:26|0|3|2",
      "extent": "3:19-3:26|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 253,
      "storage": 1
    }]
}
*/
