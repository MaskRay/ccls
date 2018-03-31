struct Wrapper {
  Wrapper(int i);
};

int called() { return 1; }

Wrapper caller() {
  return called();
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 13611487872560323389,
      "detailed_name": "Wrapper",
      "short_name": "Wrapper",
      "kind": 23,
      "declarations": ["2:3-2:10|-1|1|4"],
      "spell": "1:8-1:15|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["2:3-2:10|0|2|4", "7:1-7:8|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 10544127002917214589,
      "detailed_name": "void Wrapper::Wrapper(int i)",
      "short_name": "Wrapper",
      "kind": 9,
      "storage": 1,
      "declarations": [{
          "spell": "2:3-2:10|0|2|1",
          "param_spellings": ["2:15-2:16"]
        }],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["8:10-8:18|2|3|288"],
      "callees": []
    }, {
      "id": 1,
      "usr": 468307235068920063,
      "detailed_name": "int called()",
      "short_name": "called",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "5:5-5:11|-1|1|2",
      "extent": "5:1-5:27|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["8:10-8:16|2|3|32"],
      "callees": []
    }, {
      "id": 2,
      "usr": 11404881820527069090,
      "detailed_name": "Wrapper caller()",
      "short_name": "caller",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "7:9-7:15|-1|1|2",
      "extent": "7:1-9:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["8:10-8:18|0|3|288", "8:10-8:16|1|3|32"]
    }],
  "vars": []
}
*/
