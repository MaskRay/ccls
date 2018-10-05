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
      "spell": "5:5-5:8|5:1-5:36|2|-1",
      "bases": [],
      "vars": [14555488990109936920, 10963664335057337329],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["1:5-1:8|1:1-1:18|1|-1", "2:5-2:8|2:1-3:16|1|-1", "4:5-4:8|4:1-4:26|1|-1"],
      "derived": [],
      "uses": []
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
      "instances": [14555488990109936920, 10963664335057337329],
      "uses": []
    }],
  "usr2var": [{
      "usr": 10963664335057337329,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "spell": "5:20-5:21|5:16-5:21|1026|-1",
      "type": 53,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 14555488990109936920,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "spell": "5:13-5:14|5:9-5:14|1026|-1",
      "type": 53,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
