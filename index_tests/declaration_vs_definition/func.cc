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
      "spell": "3:6-3:9|3:1-3:14|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["1:6-1:9|1:1-1:11|1|-1", "2:6-2:9|2:1-2:11|1|-1", "4:6-4:9|4:1-4:11|1|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [],
  "usr2var": []
}
*/
