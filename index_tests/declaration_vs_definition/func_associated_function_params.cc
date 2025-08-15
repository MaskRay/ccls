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
      "vars": [7892962471424670909, 17130001153697799035, 16879535361702603834, 10198518325066875844,
14555488990109936920, 10963664335057337329], "callees": [], "kind": 12, "parent_kind": 1, "storage": 0, "declarations":
["1:5-1:8|1:1-1:18|1|-1", "2:5-2:8|2:1-3:16|1|-1", "4:5-4:8|4:1-4:26|1|-1"], "derived": [], "uses": []
    }],
  "usr2type": [{
      "usr": 452,
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
      "instances": [7892962471424670909, 17130001153697799035, 16879535361702603834, 10198518325066875844,
14555488990109936920, 10963664335057337329], "uses": []
    }],
  "usr2var": [{
      "usr": 7892962471424670909,
      "detailed_name": "int aa",
      "qual_name_offset": 4,
      "short_name": "aa",
      "spell": "2:13-2:15|2:9-2:15|1026|-1",
      "type": 452,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 10198518325066875844,
      "detailed_name": "int bbb",
      "qual_name_offset": 4,
      "short_name": "bbb",
      "spell": "4:22-4:25|4:18-4:25|1026|-1",
      "type": 452,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 10963664335057337329,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "spell": "5:20-5:21|5:16-5:21|1026|-1",
      "type": 452,
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
      "type": 452,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 16879535361702603834,
      "detailed_name": "int aaa",
      "qual_name_offset": 4,
      "short_name": "aaa",
      "spell": "4:13-4:16|4:9-4:16|1026|-1",
      "type": 452,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 17130001153697799035,
      "detailed_name": "int bb",
      "qual_name_offset": 4,
      "short_name": "bb",
      "spell": "3:13-3:15|3:9-3:15|1026|-1",
      "type": 452,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
