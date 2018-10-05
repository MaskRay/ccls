typedef int (func)(const int *a, const int *b);
static func	g;

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 8105378401105136463,
      "detailed_name": "static int g(const int *, const int *)",
      "qual_name_offset": 11,
      "short_name": "g",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["2:13-2:14|2:1-2:14|1|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 10383876566159302459,
      "detailed_name": "typedef int (func)(const int *, const int *)",
      "qual_name_offset": 12,
      "short_name": "func",
      "spell": "1:14-1:18|1:1-1:47|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 252,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["2:8-2:12|4|-1"]
    }],
  "usr2var": []
}
*/