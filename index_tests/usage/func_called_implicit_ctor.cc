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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 468307235068920063,
      "detailed_name": "int called()",
      "qual_name_offset": 4,
      "short_name": "called",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "5:5-5:11|0|1|2|-1",
      "extent": "5:1-5:27|0|1|0|-1",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["8:10-8:16|11404881820527069090|3|16420|-1"],
      "callees": []
    }, {
      "usr": 10544127002917214589,
      "detailed_name": "Wrapper::Wrapper(int i)",
      "qual_name_offset": 0,
      "short_name": "Wrapper",
      "kind": 9,
      "storage": 0,
      "declarations": ["2:3-2:10|2:3-2:17|13611487872560323389|2|1025|-1"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["8:10-8:16|11404881820527069090|3|16676|-1"],
      "callees": []
    }, {
      "usr": 11404881820527069090,
      "detailed_name": "Wrapper caller()",
      "qual_name_offset": 8,
      "short_name": "caller",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "7:9-7:15|0|1|2|-1",
      "extent": "7:1-9:2|0|1|0|-1",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["8:10-8:16|10544127002917214589|3|16676", "8:10-8:16|468307235068920063|3|16420"]
    }],
  "usr2type": [{
      "usr": 13611487872560323389,
      "detailed_name": "struct Wrapper {}",
      "qual_name_offset": 7,
      "short_name": "Wrapper",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:15|0|1|2|-1",
      "extent": "1:1-3:2|0|1|0|-1",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [10544127002917214589],
      "vars": [],
      "instances": [],
      "uses": ["2:3-2:10|13611487872560323389|2|4|-1", "7:1-7:8|0|1|4|-1"]
    }],
  "usr2var": []
}
*/
