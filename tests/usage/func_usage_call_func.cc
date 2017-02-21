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
      "definition": "1:1:6",
      "callers": ["1@1:3:3"],
      "all_uses": ["1:1:6", "1:3:3"]
    }, {
      "id": 1,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "qualified_name": "caller",
      "definition": "1:2:6",
      "callees": ["0@1:3:3"],
      "all_uses": ["1:2:6"]
    }],
  "variables": []
}
*/