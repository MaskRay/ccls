void foo() {
  int x;
  x = 3;
}
/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:1:6",
      "uses": ["1:1:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:var_usage_local.cc@16@F@foo#@x",
      "short_name": "x",
      "qualified_name": "x",
      "definition": "1:2:7",
      "uses": ["1:2:7", "1:3:3"]
    }]
}
*/
