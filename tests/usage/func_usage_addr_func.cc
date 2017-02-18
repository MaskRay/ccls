void consume(void*) {}

void used() {}

void user() {
  auto x = &used;
  consume(&used);
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "short_name": "",
      "qualified_name": "",
      "declaration": null
    }],
  "functions": [{
      "id": 0,
      "short_name": "consume",
      "qualified_name": "consume",
      "declaration": null,
      "definition": "tests/usage/func_usage_addr_func.cc:1:6",
      "declaring_type": null,
      "base": null,
      "derived": [],
      "locals": [],
      "callers": ["2@tests/usage/func_usage_addr_func.cc:7:3"],
      "callees": [],
      "uses": ["tests/usage/func_usage_addr_func.cc:7:3"]
    }, {
      "id": 1,
      "short_name": "used",
      "qualified_name": "used",
      "declaration": null,
      "definition": "tests/usage/func_usage_addr_func.cc:3:6",
      "declaring_type": null,
      "base": null,
      "derived": [],
      "locals": [],
      "callers": ["2@tests/usage/func_usage_addr_func.cc:6:13", "2@tests/usage/func_usage_addr_func.cc:7:12"],
      "callees": [],
      "uses": ["tests/usage/func_usage_addr_func.cc:6:13", "tests/usage/func_usage_addr_func.cc:7:12"]
    }, {
      "id": 2,
      "short_name": "user",
      "qualified_name": "user",
      "declaration": null,
      "definition": "tests/usage/func_usage_addr_func.cc:5:6",
      "declaring_type": null,
      "base": null,
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["1@tests/usage/func_usage_addr_func.cc:6:13", "0@tests/usage/func_usage_addr_func.cc:7:3", "1@tests/usage/func_usage_addr_func.cc:7:12"],
      "uses": []
    }],
  "variables": [{
      "id": 0,
      "short_name": "",
      "qualified_name": "",
      "declaration": "tests/usage/func_usage_addr_func.cc:1:19",
      "initializations": ["tests/usage/func_usage_addr_func.cc:1:19"],
      "variable_type": 0,
      "declaring_type": null,
      "uses": []
    }, {
      "id": 1,
      "short_name": "x",
      "qualified_name": "x",
      "declaration": "tests/usage/func_usage_addr_func.cc:6:8",
      "initializations": ["tests/usage/func_usage_addr_func.cc:6:8"],
      "variable_type": 0,
      "declaring_type": null,
      "uses": []
    }]
}
*/