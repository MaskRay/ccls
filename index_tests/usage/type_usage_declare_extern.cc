struct T {};

extern T t;
/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 5673439900521455039,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "T",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:9|-1|1|2",
      "extent": "1:1-1:12|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["3:8-3:9|-1|1|4"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 1346710425945444872,
      "detailed_name": "T t",
      "qual_name_offset": 2,
      "short_name": "t",
      "declarations": ["3:10-3:11|-1|1|1"],
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 2
    }]
}
*/
