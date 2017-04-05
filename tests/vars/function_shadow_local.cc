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
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-9:2",
      "uses": ["1:6-1:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:function_shadow_local.cc@16@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition_spelling": "2:7-2:8",
      "definition_extent": "2:3-2:8",
      "uses": ["2:7-2:8", "3:3-3:4", "8:3-8:4"]
    }, {
      "id": 1,
      "usr": "c:function_shadow_local.cc@43@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition_spelling": "5:9-5:10",
      "definition_extent": "5:5-5:10",
      "uses": ["5:9-5:10", "6:5-6:6"]
    }]
}
*/
