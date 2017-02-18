void called() {}
void caller() {
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
      "definition": "tests/usage/func_usage_call_func.cc:1:6",
      "callers": ["1@tests/usage/func_usage_call_func.cc:3:3"],
      "uses": ["tests/usage/func_usage_call_func.cc:3:3"]
    }, {
      "id": 1,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "qualified_name": "caller",
      "definition": "tests/usage/func_usage_call_func.cc:2:6",
      "callees": ["0@tests/usage/func_usage_call_func.cc:3:3"]
    }],
  "variables": []
}
*/