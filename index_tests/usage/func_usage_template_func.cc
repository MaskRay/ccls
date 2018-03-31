template<typename T>
void accept(T);

void foo() {
  accept(1);
  accept(true);
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 13420564603121289209,
      "detailed_name": "T",
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "1:19-1:20|0|3|2",
      "extent": "1:10-1:20|0|3|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:13-2:14|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 10585861037135727329,
      "detailed_name": "void accept(T)",
      "short_name": "accept",
      "kind": 12,
      "storage": 1,
      "declarations": [{
          "spell": "2:6-2:12|-1|1|1",
          "param_spellings": ["2:14-2:14"]
        }],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["5:3-5:9|1|3|32", "6:3-6:9|1|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "4:6-4:9|-1|1|2",
      "extent": "4:1-7:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["5:3-5:9|0|3|32", "6:3-6:9|0|3|32"]
    }],
  "vars": []
}
*/
