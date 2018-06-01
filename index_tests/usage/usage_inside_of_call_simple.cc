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
  "usr2func": [{
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "5:6-5:9|0|1|2",
      "extent": "5:1-7:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["6:3-6:9|18319417758892371313|3|32", "6:10-6:13|11404602816585117695|3|32", "6:18-6:21|11404602816585117695|3|32"]
    }, {
      "usr": 11404602816585117695,
      "detailed_name": "int gen()",
      "qual_name_offset": 4,
      "short_name": "gen",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "3:5-3:8|0|1|2",
      "extent": "3:1-3:24|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:10-6:13|4259594751088586730|3|32", "6:18-6:21|4259594751088586730|3|32"],
      "callees": []
    }, {
      "usr": 18319417758892371313,
      "detailed_name": "void called(int a)",
      "qual_name_offset": 5,
      "short_name": "called",
      "kind": 12,
      "storage": 0,
      "declarations": ["1:6-1:12|0|1|1"],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:3-6:9|4259594751088586730|3|32"],
      "callees": []
    }],
  "usr2type": [],
  "usr2var": []
}
*/
