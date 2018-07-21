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
      "kind": 12,
      "storage": 0,
      "declarations": ["1:6-1:9|0|1|1"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["4:3-4:6|6767773193109753523|3|16420"],
      "callees": []
    }, {
      "usr": 6767773193109753523,
      "detailed_name": "void usage()",
      "qual_name_offset": 5,
      "short_name": "usage",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "3:6-3:11|0|1|2",
      "extent": "3:1-5:2|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["4:3-4:6|4259594751088586730|3|16420"]
    }],
  "usr2type": [],
  "usr2var": []
}
*/
