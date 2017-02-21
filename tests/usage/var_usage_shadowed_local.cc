void foo() {
  int a;
  a = 1;
  {
    int a;
    a = 2;
  }
  a = 3;
}
/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:1:6",
      "all_uses": ["1:1:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:var_usage_shadowed_local.cc@16@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:2:7",
      "all_uses": ["1:2:7", "1:3:3", "1:8:3"]
    }, {
      "id": 1,
      "usr": "c:var_usage_shadowed_local.cc@43@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:5:9",
      "all_uses": ["1:5:9", "1:6:5"]
    }]
}
*/