void consume(void*) {}

void used() {}

void user() {
  auto x = &used;
  consume(&used);
}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@F@consume#*v#",
      "short_name": "consume",
      "qualified_name": "consume",
      "definition": "tests/usage/func_usage_addr_func.cc:1:6",
      "callers": ["2@tests/usage/func_usage_addr_func.cc:7:3"],
      "all_uses": ["tests/usage/func_usage_addr_func.cc:1:6", "tests/usage/func_usage_addr_func.cc:7:3"]
    }, {
      "id": 1,
      "usr": "c:@F@used#",
      "short_name": "used",
      "qualified_name": "used",
      "definition": "tests/usage/func_usage_addr_func.cc:3:6",
      "callers": ["2@tests/usage/func_usage_addr_func.cc:6:13", "2@tests/usage/func_usage_addr_func.cc:7:12"],
      "all_uses": ["tests/usage/func_usage_addr_func.cc:3:6", "tests/usage/func_usage_addr_func.cc:6:13", "tests/usage/func_usage_addr_func.cc:7:12"]
    }, {
      "id": 2,
      "usr": "c:@F@user#",
      "short_name": "user",
      "qualified_name": "user",
      "definition": "tests/usage/func_usage_addr_func.cc:5:6",
      "callees": ["1@tests/usage/func_usage_addr_func.cc:6:13", "0@tests/usage/func_usage_addr_func.cc:7:3", "1@tests/usage/func_usage_addr_func.cc:7:12"],
      "all_uses": ["tests/usage/func_usage_addr_func.cc:5:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:func_usage_addr_func.cc@61@F@user#@x",
      "short_name": "x",
      "qualified_name": "x",
      "definition": "tests/usage/func_usage_addr_func.cc:6:8",
      "all_uses": ["tests/usage/func_usage_addr_func.cc:6:8"]
    }]
}
*/