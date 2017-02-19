void called() {}

void caller() {
  auto x = &called;
  x();

  called();
}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@F@called#",
      "short_name": "called",
      "qualified_name": "called",
      "definition": "tests/usage/var_usage_call_function.cc:1:6",
      "callers": ["1@tests/usage/var_usage_call_function.cc:4:13", "1@tests/usage/var_usage_call_function.cc:7:3"],
      "uses": ["tests/usage/var_usage_call_function.cc:4:13", "tests/usage/var_usage_call_function.cc:7:3"]
    }, {
      "id": 1,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "qualified_name": "caller",
      "definition": "tests/usage/var_usage_call_function.cc:3:6",
      "callees": ["0@tests/usage/var_usage_call_function.cc:4:13", "0@tests/usage/var_usage_call_function.cc:7:3"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:var_usage_call_function.cc@39@F@caller#@x",
      "short_name": "x",
      "qualified_name": "x",
      "declaration": "tests/usage/var_usage_call_function.cc:4:8",
      "initializations": ["tests/usage/var_usage_call_function.cc:4:8"],
      "uses": ["tests/usage/var_usage_call_function.cc:5:3"]
    }]
}
*/