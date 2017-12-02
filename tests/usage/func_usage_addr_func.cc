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
      "is_operator": false,
      "usr": "c:@F@consume#*v#",
      "short_name": "consume",
      "detailed_name": "void consume(void *)",
      "definition_spelling": "1:6-1:13",
      "definition_extent": "1:1-1:23",
      "callers": ["2@7:3-7:10"]
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@F@used#",
      "short_name": "used",
      "detailed_name": "void used()",
      "definition_spelling": "3:6-3:10",
      "definition_extent": "3:1-3:15",
      "callers": ["2@6:13-6:17", "2@7:12-7:16"]
    }, {
      "id": 2,
      "is_operator": false,
      "usr": "c:@F@user#",
      "short_name": "user",
      "detailed_name": "void user()",
      "definition_spelling": "5:6-5:10",
      "definition_extent": "5:1-8:2",
      "callees": ["1@6:13-6:17", "0@7:3-7:10", "1@7:12-7:16"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:func_usage_addr_func.cc@61@F@user#@x",
      "short_name": "x",
      "detailed_name": "void (*)() x",
      "definition_spelling": "6:8-6:9",
      "definition_extent": "6:3-6:17",
      "is_local": true,
      "is_macro": false,
      "uses": ["6:8-6:9"]
    }]
}
*/
