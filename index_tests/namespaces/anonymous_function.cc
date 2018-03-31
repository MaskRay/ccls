namespace {
void foo();
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 7144845543074395457,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 5010253035933134245,
      "detailed_name": "void (anon)::foo()",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [{
          "spell": "2:6-2:9|0|2|1",
          "param_spellings": []
        }],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
*/
