#define FOO(aaa, bbb) \
  int a();\
  int a() { return aaa + bbb; }


int make1() {
  return 3;
}
const int make2 = 5;


FOO(make1(), make2);

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 9720930732776154610,
      "detailed_name": "int a()",
      "qual_name_offset": 4,
      "short_name": "a",
      "kind": 12,
      "storage": 0,
      "declarations": ["12:1-12:20|0|1|1"],
      "spell": "12:1-12:20|0|1|2",
      "extent": "1:1-1:1|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["2:7-2:8|0|1|64|0", "3:7-3:8|0|1|64|0"],
      "callees": []
    }, {
      "usr": 14400399977994209582,
      "detailed_name": "int make1()",
      "qual_name_offset": 4,
      "short_name": "make1",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "6:5-6:10|0|1|2",
      "extent": "6:1-8:2|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["12:5-12:10|0|1|16420", "12:5-12:10|0|1|64|0"],
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
      "instances": [2878407290385495202],
      "uses": []
    }],
  "usr2var": [{
      "usr": 2878407290385495202,
      "detailed_name": "const int make2",
      "qual_name_offset": 10,
      "short_name": "make2",
      "hover": "const int make2 = 5",
      "declarations": [],
      "spell": "9:11-9:16|0|1|2",
      "extent": "9:1-9:20|0|1|0",
      "type": 53,
      "uses": ["12:14-12:19|0|1|12", "12:14-12:19|0|1|64|0"],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 14219599523415845943,
      "detailed_name": "FOO",
      "qual_name_offset": 0,
      "short_name": "FOO",
      "hover": "#define FOO(aaa, bbb) \\\n  int a();\\\n  int a() { return aaa + bbb; }",
      "declarations": [],
      "spell": "1:9-1:12|0|1|2",
      "extent": "1:9-3:32|0|1|0",
      "type": 0,
      "uses": ["12:1-12:4|0|1|64"],
      "kind": 255,
      "storage": 0
    }]
}
*/