struct T {};

extern T t;
/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 5673439900521455039,
      "detailed_name": "struct T {}",
      "qual_name_offset": 7,
      "short_name": "T",
      "spell": "1:8-1:9|1:1-1:12|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [1346710425945444872],
      "uses": ["3:8-3:9|4|-1"]
    }],
  "usr2var": [{
      "usr": 1346710425945444872,
      "detailed_name": "extern T t",
      "qual_name_offset": 9,
      "short_name": "t",
      "type": 5673439900521455039,
      "kind": 13,
      "parent_kind": 0,
      "storage": 1,
      "declarations": ["3:10-3:11|3:1-3:11|1|-1"],
      "uses": []
    }]
}
*/
