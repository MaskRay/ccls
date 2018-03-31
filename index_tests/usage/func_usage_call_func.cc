void called() {}
void caller() {
  called();
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "usr": 468307235068920063,
      "detailed_name": "void called()",
      "short_name": "called",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "1:6-1:12|-1|1|2",
      "extent": "1:1-1:17|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["3:3-3:9|1|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 11404881820527069090,
      "detailed_name": "void caller()",
      "short_name": "caller",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "2:6-2:12|-1|1|2",
      "extent": "2:1-4:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["3:3-3:9|0|3|32"]
    }],
  "vars": []
}
*/
