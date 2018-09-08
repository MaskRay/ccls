struct Type {};
static Type t;
/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 13487927231218873822,
      "detailed_name": "struct Type {}",
      "qual_name_offset": 7,
      "short_name": "Type",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:12|0|1|2|-1",
      "extent": "1:1-1:15|0|1|0|-1",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [6601831367240627080],
      "uses": ["2:8-2:12|0|1|4|-1"]
    }],
  "usr2var": [{
      "usr": 6601831367240627080,
      "detailed_name": "static Type t",
      "qual_name_offset": 12,
      "short_name": "t",
      "declarations": [],
      "spell": "2:13-2:14|0|1|2|-1",
      "extent": "2:1-2:14|0|1|0|-1",
      "type": 13487927231218873822,
      "uses": [],
      "kind": 13,
      "storage": 2
    }]
}
*/
