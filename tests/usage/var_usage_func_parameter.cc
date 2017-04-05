void foo(int a) {
  a += 10;
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
      "usr": "c:var_usage_func_parameter.cc@9@F@foo#I#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:14",
      "uses": ["1:14", "2:3"]
    }]
}
*/
