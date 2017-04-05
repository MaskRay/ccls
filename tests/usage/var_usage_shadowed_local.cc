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
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:6",
      "uses": ["1:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:var_usage_shadowed_local.cc@16@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "2:7",
      "uses": ["2:7", "3:3", "8:3"]
    }, {
      "id": 1,
      "usr": "c:var_usage_shadowed_local.cc@43@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "5:9",
      "uses": ["5:9", "6:5"]
    }]
}
*/
