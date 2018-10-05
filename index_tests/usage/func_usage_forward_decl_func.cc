void foo();

void usage() {
  foo();
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
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["1:6-1:9|1:1-1:11|1|-1"],
      "derived": [],
      "uses": ["4:3-4:6|16420|-1"]
    }, {
      "usr": 6767773193109753523,
      "detailed_name": "void usage()",
      "qual_name_offset": 5,
      "short_name": "usage",
      "spell": "3:6-3:11|3:1-5:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": ["4:3-4:6|4259594751088586730|3|16420"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [],
  "usr2var": []
}
*/
