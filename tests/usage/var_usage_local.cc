void foo() {
  int x;
  x = 3;
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
      "usr": "c:var_usage_local.cc@16@F@foo#@x",
      "short_name": "x",
      "qualified_name": "x",
      "definition": "1:2:7",
      "all_uses": ["1:2:7", "1:3:3"]
    }]
}
*/