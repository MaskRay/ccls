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
      "kind": 12,
      "storage": 0,
      "declarations": ["2:13-2:14|0|1|1"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 10383876566159302459,
      "detailed_name": "typedef int (func)(const int *, const int *)",
      "qual_name_offset": 12,
      "short_name": "func",
      "kind": 252,
      "declarations": [],
      "spell": "1:14-1:18|0|1|2",
      "extent": "1:1-1:47|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:8-2:12|0|1|4"]
    }],
  "usr2var": []
}
*/