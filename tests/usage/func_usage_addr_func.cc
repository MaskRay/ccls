void consume(void*) {}

void used() {}

void user() {
  auto x = &used;
  consume(&used);
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@consume#*v#",
      "short_name": "consume",
      "qualified_name": "consume",
      "definition_spelling": "1:6-1:13",
      "definition_extent": "1:1-1:23",
      "callers": ["2@7:3-7:10"],
      "uses": ["1:6-1:13", "7:3-7:10"]
    }, {
      "id": 1,
      "usr": "c:@F@used#",
      "short_name": "used",
      "qualified_name": "used",
      "definition_spelling": "3:6-3:10",
      "definition_extent": "3:1-3:15",
      "callers": ["2@6:13-6:17", "2@7:12-7:16"],
      "uses": ["3:6-3:10", "6:13-6:17", "7:12-7:16"]
    }, {
      "id": 2,
      "usr": "c:@F@user#",
      "short_name": "user",
      "qualified_name": "user",
      "definition_spelling": "5:6-5:10",
      "definition_extent": "5:1-8:2",
      "callees": ["1@6:13-6:17", "0@7:3-7:10", "1@7:12-7:16"],
      "uses": ["5:6-5:10"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:func_usage_addr_func.cc@61@F@user#@x",
      "short_name": "x",
      "qualified_name": "x",
      "definition_spelling": "6:8-6:9",
      "definition_extent": "6:3-6:17",
      "uses": ["6:8-6:9"]
    }]
}
*/
