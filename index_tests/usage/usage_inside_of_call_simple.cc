void called(int a);

int gen() { return 1; }

void foo() {
  called(gen() * gen());
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "5:6-5:9|5:1-7:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": ["6:3-6:9|18319417758892371313|3|16420", "6:10-6:13|11404602816585117695|3|16420", "6:18-6:21|11404602816585117695|3|16420"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 11404602816585117695,
      "detailed_name": "int gen()",
      "qual_name_offset": 4,
      "short_name": "gen",
      "spell": "3:5-3:8|3:1-3:24|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["6:10-6:13|16420|-1", "6:18-6:21|16420|-1"]
    }, {
      "usr": 18319417758892371313,
      "detailed_name": "void called(int a)",
      "qual_name_offset": 5,
      "short_name": "called",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["1:6-1:12|1:1-1:19|1|-1"],
      "derived": [],
      "uses": ["6:3-6:9|16420|-1"]
    }],
  "usr2type": [],
  "usr2var": []
}
*/
