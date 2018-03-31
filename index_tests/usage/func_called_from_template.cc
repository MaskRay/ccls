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
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "usr": 468307235068920063,
      "detailed_name": "void called()",
      "short_name": "called",
      "kind": 12,
      "storage": 1,
      "declarations": [{
          "spell": "1:6-1:12|-1|1|1",
          "param_spellings": []
        }],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["5:3-5:9|1|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 10177235824697315808,
      "detailed_name": "void caller()",
      "short_name": "caller",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "4:6-4:12|-1|1|2",
      "extent": "4:1-6:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["9:3-9:9|2|3|32"],
      "callees": ["5:3-5:9|0|3|32"]
    }, {
      "id": 2,
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "8:6-8:9|-1|1|2",
      "extent": "8:1-10:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["9:3-9:9|1|3|32"]
    }],
  "vars": []
}
*/