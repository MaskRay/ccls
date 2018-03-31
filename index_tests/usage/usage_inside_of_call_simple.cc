void called(int a);

int gen() { return 1; }

void foo() {
  called(gen() * gen());
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "usr": 18319417758892371313,
      "detailed_name": "void called(int a)",
      "short_name": "called",
      "kind": 12,
      "storage": 1,
      "declarations": [{
          "spell": "1:6-1:12|-1|1|1",
          "param_spellings": ["1:17-1:18"]
        }],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:3-6:9|2|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 11404602816585117695,
      "detailed_name": "int gen()",
      "short_name": "gen",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "3:5-3:8|-1|1|2",
      "extent": "3:1-3:24|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:10-6:13|2|3|32", "6:18-6:21|2|3|32"],
      "callees": []
    }, {
      "id": 2,
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "5:6-5:9|-1|1|2",
      "extent": "5:1-7:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["6:3-6:9|0|3|32", "6:10-6:13|1|3|32", "6:18-6:21|1|3|32"]
    }],
  "vars": []
}
*/
