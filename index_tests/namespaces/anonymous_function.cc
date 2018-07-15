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
      "kind": 12,
      "storage": 0,
      "declarations": ["2:6-2:9|7144845543074395457|2|1"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 7144845543074395457,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [5010253035933134245],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
