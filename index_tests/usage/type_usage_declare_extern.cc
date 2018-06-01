struct T {};

extern T t;
/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 5673439900521455039,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "T",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:9|0|1|2",
      "extent": "1:1-1:12|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [1346710425945444872],
      "uses": ["3:8-3:9|0|1|4"]
    }],
  "usr2var": [{
      "usr": 1346710425945444872,
      "detailed_name": "T t",
      "qual_name_offset": 2,
      "short_name": "t",
      "declarations": ["3:10-3:11|0|1|1"],
      "type": 5673439900521455039,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
