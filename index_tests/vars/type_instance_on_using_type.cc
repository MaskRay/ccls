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
      "spell": "3:6-3:9|3:1-5:2|2|-1",
      "bases": [],
      "vars": [6975456769752895964],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 4750332761459066907,
      "detailed_name": "struct S {}",
      "qual_name_offset": 7,
      "short_name": "S",
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
      "instances": [],
      "uses": ["2:11-2:12|4|-1"]
    }, {
      "usr": 7434820806199665424,
      "detailed_name": "using F = S",
      "qual_name_offset": 6,
      "short_name": "F",
      "spell": "2:7-2:8|2:1-2:12|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 4750332761459066907,
      "kind": 252,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [6975456769752895964],
      "uses": ["4:3-4:4|4|-1"]
    }],
  "usr2var": [{
      "usr": 6975456769752895964,
      "detailed_name": "F a",
      "qual_name_offset": 2,
      "short_name": "a",
      "spell": "4:5-4:6|4:3-4:6|2|-1",
      "type": 7434820806199665424,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
