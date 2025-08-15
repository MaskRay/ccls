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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "4:6-4:9|4:1-7:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": ["5:3-5:9|10585861037135727329|3|16420", "6:3-6:9|10585861037135727329|3|16420"],
      "kind": 12,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 10585861037135727329,
      "detailed_name": "void accept(T)",
      "qual_name_offset": 5,
      "short_name": "accept",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["2:6-2:12|2:1-2:15|1|-1"],
      "derived": [],
      "uses": ["5:3-5:9|16420|-1", "6:3-6:9|16420|-1"]
    }],
  "usr2type": [{
      "usr": 13420564603121289209,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 26,
      "parent_kind": 0,
      "declarations": ["1:19-1:20|1:10-1:20|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["2:13-2:14|4|-1"]
    }],
  "usr2var": []
}
*/
