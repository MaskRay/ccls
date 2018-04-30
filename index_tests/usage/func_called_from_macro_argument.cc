#define MACRO_CALL(e) e

bool called(bool a, bool b);

void caller() {
  MACRO_CALL(called(true, true));
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 3787803219955606747,
      "detailed_name": "bool called(bool a, bool b)",
      "qual_name_offset": 5,
      "short_name": "called",
      "kind": 12,
      "storage": 1,
      "declarations": ["3:6-3:12|0|1|1"],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:14-6:20|11404881820527069090|3|32"],
      "callees": []
    }, {
      "usr": 11404881820527069090,
      "detailed_name": "void caller()",
      "qual_name_offset": 5,
      "short_name": "caller",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "5:6-5:12|0|1|2",
      "extent": "5:1-7:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["6:14-6:20|3787803219955606747|3|32"]
    }],
  "usr2type": [],
  "usr2var": [{
      "usr": 1290746656694198202,
      "detailed_name": "MACRO_CALL",
      "qual_name_offset": 0,
      "short_name": "MACRO_CALL",
      "hover": "#define MACRO_CALL(e) e",
      "declarations": [],
      "spell": "1:9-1:19|0|1|2",
      "extent": "1:9-1:24|0|1|0",
      "type": 0,
      "uses": ["6:3-6:13|0|1|4"],
      "kind": 255,
      "storage": 0
    }]
}
*/