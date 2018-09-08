void foo();
void foo();
void foo() {}
void foo();

/*
// Note: we always use the latest seen ("most local") definition/declaration.
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
      "declarations": ["1:6-1:9|1:1-1:11|0|1|1|-1", "2:6-2:9|2:1-2:11|0|1|1|-1", "4:6-4:9|4:1-4:11|0|1|1|-1"],
      "spell": "3:6-3:9|0|1|2|-1",
      "extent": "3:1-3:14|0|1|0|-1",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [],
  "usr2var": []
}
*/
