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
      "spell": "12:1-12:20|12:1-12:4|2|-1",
      "bases": [],
      "vars": [],
      "callees": ["12:5-12:10|14400399977994209582|3|16420"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["12:1-12:20|12:1-12:4|1|-1"],
      "derived": [],
      "uses": ["2:7-2:8|64|0", "3:7-3:8|64|0"]
    }, {
      "usr": 14400399977994209582,
      "detailed_name": "int make1()",
      "qual_name_offset": 4,
      "short_name": "make1",
      "spell": "6:5-6:10|6:1-8:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["12:5-12:10|16420|-1", "12:5-12:10|64|0"]
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
      "instances": [2878407290385495202],
      "uses": []
    }],
  "usr2var": [{
      "usr": 2878407290385495202,
      "detailed_name": "const int make2",
      "qual_name_offset": 10,
      "short_name": "make2",
      "hover": "const int make2 = 5",
      "spell": "9:11-9:16|9:1-9:20|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": ["12:14-12:19|12|-1", "12:14-12:19|64|0"]
    }, {
      "usr": 14219599523415845943,
      "detailed_name": "FOO",
      "qual_name_offset": 0,
      "short_name": "FOO",
      "hover": "#define FOO(aaa, bbb) \\\n  int a();\\\n  int a() { return aaa + bbb; }",
      "spell": "1:9-1:12|1:9-3:32|2|-1",
      "type": 0,
      "kind": 255,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "uses": ["12:1-12:4|64|-1"]
    }]
}
*/