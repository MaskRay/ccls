void foo(int a) {
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
      "usr": "c:@F@foo#I#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:6",
      "uses": ["1:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:var_usage_shadowed_parameter.cc@9@F@foo#I#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:14",
      "uses": ["1:14", "2:3", "7:3"]
    }, {
      "id": 1,
      "usr": "c:var_usage_shadowed_parameter.cc@38@F@foo#I#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "4:9",
      "uses": ["4:9", "5:5"]
    }]
}
*/
