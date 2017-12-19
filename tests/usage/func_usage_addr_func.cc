void consume(void*) {}

void used() {}

void user() {
  auto x = &used;
  consume(&used);
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@consume#*v#",
      "short_name": "consume",
      "detailed_name": "void consume(void *)",
      "hover": "void consume(void *)",
      "declarations": [],
      "definition_spelling": "1:6-1:13",
      "definition_extent": "1:1-1:23",
      "derived": [],
      "locals": [],
      "callers": ["2@7:3-7:10"],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@F@used#",
      "short_name": "used",
      "detailed_name": "void used()",
      "hover": "void used()",
      "declarations": [],
      "definition_spelling": "3:6-3:10",
      "definition_extent": "3:1-3:15",
      "derived": [],
      "locals": [],
      "callers": ["2@6:13-6:17", "2@7:12-7:16"],
      "callees": []
    }, {
      "id": 2,
      "is_operator": false,
      "usr": "c:@F@user#",
      "short_name": "user",
      "detailed_name": "void user()",
      "hover": "void user()",
      "declarations": [],
      "definition_spelling": "5:6-5:10",
      "definition_extent": "5:1-8:2",
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["1@6:13-6:17", "0@7:3-7:10", "1@7:12-7:16"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:func_usage_addr_func.cc@61@F@user#@x",
      "short_name": "x",
      "detailed_name": "void (*)() x",
      "hover": "void (*)()",
      "definition_spelling": "6:8-6:9",
      "definition_extent": "6:3-6:17",
      "is_local": true,
      "is_macro": false,
      "uses": ["6:8-6:9"]
    }]
}
*/
