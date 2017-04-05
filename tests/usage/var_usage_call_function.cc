void called() {}

void caller() {
  auto x = &called;
  x();

  called();
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@called#",
      "short_name": "called",
      "qualified_name": "called",
      "definition": "1:6",
      "callers": ["1@4:13", "1@7:3"],
      "uses": ["1:6", "4:13", "7:3"]
    }, {
      "id": 1,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "qualified_name": "caller",
      "definition": "3:6",
      "callees": ["0@4:13", "0@7:3"],
      "uses": ["3:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:var_usage_call_function.cc@39@F@caller#@x",
      "short_name": "x",
      "qualified_name": "x",
      "definition": "4:8",
      "uses": ["4:8", "5:3"]
    }]
}
*/
