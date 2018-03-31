void foo(int a, int b);

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "usr": 2747674671862363334,
      "detailed_name": "void foo(int a, int b)",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [{
          "spell": "1:6-1:9|-1|1|1",
          "param_spellings": ["1:14-1:15", "1:21-1:22"]
        }],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
*/
