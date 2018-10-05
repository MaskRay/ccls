void consume(void (*)()) {}

void used() {}

void user() {
  void (*x)() = &used;
  consume(&used);
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 5264867802674151787,
      "detailed_name": "void used()",
      "qual_name_offset": 5,
      "short_name": "used",
      "spell": "3:6-3:10|3:1-3:15|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["6:18-6:22|132|-1", "7:12-7:16|132|-1"]
    }, {
      "usr": 9376923949268137283,
      "detailed_name": "void user()",
      "qual_name_offset": 5,
      "short_name": "user",
      "spell": "5:6-5:10|5:1-8:2|2|-1",
      "bases": [],
      "vars": [16088407831770615719],
      "callees": ["6:18-6:22|5264867802674151787|3|132", "6:18-6:22|5264867802674151787|3|132", "7:3-7:10|12924914488846929470|3|16420", "7:12-7:16|5264867802674151787|3|132"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 12924914488846929470,
      "detailed_name": "void consume(void (*)())",
      "qual_name_offset": 5,
      "short_name": "consume",
      "spell": "1:6-1:13|1:1-1:28|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["7:3-7:10|16420|-1"]
    }],
  "usr2type": [],
  "usr2var": [{
      "usr": 16088407831770615719,
      "detailed_name": "void (*x)()",
      "qual_name_offset": 7,
      "short_name": "x",
      "hover": "void (*x)() = &used",
      "spell": "6:10-6:11|6:3-6:22|2|-1",
      "type": 0,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
