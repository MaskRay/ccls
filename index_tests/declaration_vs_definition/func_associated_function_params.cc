int foo(int, int);
int foo(int aa,
        int bb);
int foo(int aaa, int bbb);
int foo(int a, int b) { return 0; }

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 17,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 2747674671862363334,
      "detailed_name": "int foo(int a, int b)",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [{
          "spell": "1:5-1:8|-1|1|1",
          "param_spellings": ["1:12-1:12", "1:17-1:17"]
        }, {
          "spell": "2:5-2:8|-1|1|1",
          "param_spellings": ["2:13-2:15", "3:13-3:15"]
        }, {
          "spell": "4:5-4:8|-1|1|1",
          "param_spellings": ["4:13-4:16", "4:22-4:25"]
        }],
      "spell": "5:5-5:8|-1|1|2",
      "extent": "5:1-5:36|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0, 1],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 10480417713467708012,
      "detailed_name": "int a",
      "short_name": "a",
      "declarations": [],
      "spell": "5:13-5:14|0|3|2",
      "extent": "5:9-5:14|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 253,
      "storage": 1
    }, {
      "id": 1,
      "usr": 18099600680625658464,
      "detailed_name": "int b",
      "short_name": "b",
      "declarations": [],
      "spell": "5:20-5:21|0|3|2",
      "extent": "5:16-5:21|0|3|0",
      "type": 0,
      "uses": [],
      "kind": 253,
      "storage": 1
    }]
}
*/
