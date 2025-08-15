#define MACRO_CALL(e) e

bool called(bool a, bool b);

void caller() {
  MACRO_CALL(called(true, true));
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 3787803219955606747,
      "detailed_name": "bool called(bool a, bool b)",
      "qual_name_offset": 5,
      "short_name": "called",
      "bases": [],
      "vars": [821688872341099790, 6986353817767193884],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["3:6-3:12|3:1-3:28|1|-1"],
      "derived": [],
      "uses": ["6:14-6:20|16420|-1", "6:14-6:20|64|0"]
    }, {
      "usr": 11404881820527069090,
      "detailed_name": "void caller()",
      "qual_name_offset": 5,
      "short_name": "caller",
      "spell": "5:6-5:12|5:1-7:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": ["6:14-6:20|3787803219955606747|3|16420"],
      "kind": 12,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 436,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [821688872341099790, 6986353817767193884],
      "uses": []
    }],
  "usr2var": [{
      "usr": 821688872341099790,
      "detailed_name": "bool a",
      "qual_name_offset": 5,
      "short_name": "a",
      "spell": "3:18-3:19|3:13-3:19|1026|-1",
      "type": 436,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 6986353817767193884,
      "detailed_name": "bool b",
      "qual_name_offset": 5,
      "short_name": "b",
      "spell": "3:26-3:27|3:21-3:27|1026|-1",
      "type": 436,
      "kind": 253,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 16326993795872073150,
      "detailed_name": "MACRO_CALL",
      "qual_name_offset": 0,
      "short_name": "MACRO_CALL",
      "hover": "#define MACRO_CALL(e) e",
      "spell": "1:9-1:19|1:9-1:24|2|-1",
      "type": 0,
      "kind": 255,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "uses": ["6:3-6:13|64|-1"]
    }]
}
*/