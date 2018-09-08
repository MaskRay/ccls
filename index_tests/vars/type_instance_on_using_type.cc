struct S {};
using F = S;
void Foo() {
  F a;
}

// TODO: Should we also add a usage to |S|?

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 4654328188330986029,
      "detailed_name": "void Foo()",
      "qual_name_offset": 5,
      "short_name": "Foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "3:6-3:9|0|1|2|-1",
      "extent": "3:1-5:2|0|1|0|-1",
      "bases": [],
      "derived": [],
      "vars": [6975456769752895964],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 4750332761459066907,
      "detailed_name": "struct S {}",
      "qual_name_offset": 7,
      "short_name": "S",
      "kind": 23,
      "declarations": [],
      "spell": "1:8-1:9|0|1|2|-1",
      "extent": "1:1-1:12|0|1|0|-1",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:11-2:12|0|1|4|-1"]
    }, {
      "usr": 7434820806199665424,
      "detailed_name": "using F = S",
      "qual_name_offset": 6,
      "short_name": "F",
      "kind": 252,
      "declarations": [],
      "spell": "2:7-2:8|0|1|2|-1",
      "extent": "2:1-2:12|0|1|0|-1",
      "alias_of": 4750332761459066907,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [6975456769752895964],
      "uses": ["4:3-4:4|4654328188330986029|3|4|-1"]
    }],
  "usr2var": [{
      "usr": 6975456769752895964,
      "detailed_name": "F a",
      "qual_name_offset": 2,
      "short_name": "a",
      "declarations": [],
      "spell": "4:5-4:6|4654328188330986029|3|2|-1",
      "extent": "4:3-4:6|4654328188330986029|3|0|-1",
      "type": 7434820806199665424,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
