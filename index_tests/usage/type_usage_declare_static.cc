struct Type {};
static Type t;
/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 13487927231218873822,
      "detailed_name": "Type",
      "short_name": "Type",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:12|-1|1|2",
      "extent": "1:1-1:15|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["2:8-2:12|-1|1|4"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 6601831367240627080,
      "detailed_name": "Type t",
      "short_name": "t",
      "declarations": [],
      "spell": "2:13-2:14|-1|1|2",
      "extent": "2:1-2:14|-1|1|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 3
    }]
}
*/
