namespace {
void foo();
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 5010253035933134245,
      "detailed_name": "void (anon ns)::foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["2:6-2:9|2:1-2:11|1|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 7144845543074395457,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [5010253035933134245],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
