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
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["1:6-1:12|1:1-1:14|1|-1"],
      "derived": [],
      "uses": ["5:3-5:9|16420|-1"]
    }, {
      "usr": 2459767597003442547,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "vars": [],
      "callees": ["5:3-5:9|468307235068920063|3|16420"],
      "kind": 0,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "8:6-8:9|8:1-10:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": ["9:3-9:9|10177235824697315808|3|16420"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 10177235824697315808,
      "detailed_name": "void caller()",
      "qual_name_offset": 5,
      "short_name": "caller",
      "spell": "4:6-4:12|4:1-6:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": ["5:3-5:9|468307235068920063|3|16420"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["9:3-9:9|16420|-1"]
    }],
  "usr2type": [],
  "usr2var": []
}
*/