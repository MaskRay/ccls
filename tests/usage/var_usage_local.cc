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
      "qualified_name": "void foo()",
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-4:2"
    }],
  "vars": [{
      "id": 0,
      "usr": "c:var_usage_local.cc@16@F@foo#@x",
      "short_name": "x",
      "qualified_name": "int x",
      "definition_spelling": "2:7-2:8",
      "definition_extent": "2:3-2:8",
      "uses": ["2:7-2:8", "3:3-3:4"]
    }]
}
*/
