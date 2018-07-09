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
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "3:6-3:10|0|1|2",
      "extent": "3:1-3:15|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:18-6:22|0|1|132", "7:12-7:16|0|1|132"],
      "callees": []
    }, {
      "usr": 9376923949268137283,
      "detailed_name": "void user()",
      "qual_name_offset": 5,
      "short_name": "user",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "5:6-5:10|0|1|2",
      "extent": "5:1-8:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [16088407831770615719],
      "uses": [],
      "callees": []
    }, {
      "usr": 12924914488846929470,
      "detailed_name": "void consume(void (*)())",
      "qual_name_offset": 5,
      "short_name": "consume",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "1:6-1:13|0|1|2",
      "extent": "1:1-1:28|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["7:3-7:10|0|1|8228"],
      "callees": []
    }],
  "usr2type": [],
  "usr2var": [{
      "usr": 16088407831770615719,
      "detailed_name": "void (*x)()",
      "qual_name_offset": 7,
      "short_name": "x",
      "hover": "void (*x)() = &used",
      "declarations": [],
      "spell": "6:10-6:11|9376923949268137283|3|2",
      "extent": "6:3-6:22|9376923949268137283|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
