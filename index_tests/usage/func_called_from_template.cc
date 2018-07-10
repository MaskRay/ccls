void called();

template <typename T>
void caller() {
  called();
}

void foo() {
  caller<int>();
}

/*
// NOTE: without caller<int>() instantation caller() is never visited so
// called() is never referenced.
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 468307235068920063,
      "detailed_name": "void called()",
      "qual_name_offset": 5,
      "short_name": "called",
      "kind": 12,
      "storage": 0,
      "declarations": ["1:6-1:12|0|1|1"],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["5:3-5:9|0|1|16420"],
      "callees": []
    }, {
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "8:6-8:9|0|1|2",
      "extent": "8:1-10:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 10177235824697315808,
      "detailed_name": "void caller()",
      "qual_name_offset": 5,
      "short_name": "caller",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "4:6-4:12|0|1|2",
      "extent": "4:1-6:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["9:3-9:9|0|1|16420"],
      "callees": []
    }],
  "usr2type": [],
  "usr2var": []
}
*/