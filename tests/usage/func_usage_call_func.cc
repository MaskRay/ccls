void called() {}
void caller() {
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
      "callers": ["1@3:3"],
      "uses": ["1:6", "3:3"]
    }, {
      "id": 1,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "qualified_name": "caller",
      "definition": "2:6",
      "callees": ["0@3:3"],
      "uses": ["2:6"]
    }]
}
*/
