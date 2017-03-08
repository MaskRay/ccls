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
      "definition": "1:1:6",
      "callers": ["2@1:7:3"],
      "uses": ["1:1:6", "1:7:3"]
    }, {
      "id": 1,
      "usr": "c:@F@used#",
      "short_name": "used",
      "qualified_name": "used",
      "definition": "1:3:6",
      "callers": ["2@1:6:13", "2@1:7:12"],
      "uses": ["1:3:6", "1:6:13", "1:7:12"]
    }, {
      "id": 2,
      "usr": "c:@F@user#",
      "short_name": "user",
      "qualified_name": "user",
      "definition": "1:5:6",
      "callees": ["1@1:6:13", "0@1:7:3", "1@1:7:12"],
      "uses": ["1:5:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:func_usage_addr_func.cc@61@F@user#@x",
      "short_name": "x",
      "qualified_name": "x",
      "definition": "1:6:8",
      "uses": ["1:6:8"]
    }]
}
*/
