void called() {}

void caller() {
  auto x = &called;
  x();

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
      "uses": ["4:13-4:19|1|3|32", "7:3-7:9|1|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 11404881820527069090,
      "detailed_name": "void caller()",
      "short_name": "caller",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "3:6-3:12|-1|1|2",
      "extent": "3:1-8:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0],
      "uses": [],
      "callees": ["4:13-4:19|0|3|32", "4:13-4:19|0|3|32", "7:3-7:9|0|3|32"]
    }],
  "vars": [{
      "id": 0,
      "usr": 3510529098767253033,
      "detailed_name": "void (*)() x",
      "short_name": "x",
      "declarations": [],
      "spell": "4:8-4:9|1|3|2",
      "extent": "4:3-4:19|1|3|0",
      "uses": ["5:3-5:4|1|3|4"],
      "kind": 13,
      "storage": 1
    }]
}
*/
