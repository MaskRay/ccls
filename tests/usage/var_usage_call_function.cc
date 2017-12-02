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
      "is_operator": false,
      "usr": "c:@F@called#",
      "short_name": "called",
      "detailed_name": "void called()",
      "definition_spelling": "1:6-1:12",
      "definition_extent": "1:1-1:17",
      "callers": ["1@4:13-4:19", "1@7:3-7:9"]
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "detailed_name": "void caller()",
      "definition_spelling": "3:6-3:12",
      "definition_extent": "3:1-8:2",
      "callees": ["0@4:13-4:19", "0@7:3-7:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:var_usage_call_function.cc@39@F@caller#@x",
      "short_name": "x",
      "detailed_name": "void (*)() x",
      "definition_spelling": "4:8-4:9",
      "definition_extent": "4:3-4:19",
      "is_local": true,
      "is_macro": false,
      "uses": ["4:8-4:9", "5:3-5:4"]
    }]
}
*/
