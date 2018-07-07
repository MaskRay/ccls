int foo(int, int);
int foo(int aa,
        int bb);
int foo(int aaa, int bbb);
int foo(int a, int b) { return 0; }

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 2747674671862363334,
      "detailed_name": "int foo(int, int)",
      "qual_name_offset": 4,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": ["1:5-1:8|0|1|1", "2:5-2:8|0|1|1", "4:5-4:8|0|1|1"],
      "spell": "5:5-5:8|0|1|2",
      "extent": "1:1-1:18|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
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
      "instances": [14555488990109936920, 10963664335057337329],
      "uses": []
    }],
  "usr2var": [{
      "usr": 10963664335057337329,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "declarations": [],
      "spell": "5:20-5:21|2747674671862363334|3|514",
      "extent": "5:16-5:21|2747674671862363334|3|0",
      "type": 53,
      "uses": [],
      "kind": 253,
      "storage": 0
    }, {
      "usr": 14555488990109936920,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "declarations": [],
      "spell": "5:13-5:14|2747674671862363334|3|514",
      "extent": "5:9-5:14|2747674671862363334|3|0",
      "type": 53,
      "uses": [],
      "kind": 253,
      "storage": 0
    }]
}
*/
