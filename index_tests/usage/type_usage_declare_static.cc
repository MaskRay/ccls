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
      "spell": "1:8-1:12|1:1-1:15|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [6601831367240627080],
      "uses": ["2:8-2:12|4|-1"]
    }],
  "usr2var": [{
      "usr": 6601831367240627080,
      "detailed_name": "static Type t",
      "qual_name_offset": 12,
      "short_name": "t",
      "spell": "2:13-2:14|2:1-2:14|2|-1",
      "type": 13487927231218873822,
      "kind": 13,
      "parent_kind": 0,
      "storage": 2,
      "declarations": [],
      "uses": []
    }]
}
*/
