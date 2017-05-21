static int a;

void foo() {
  a = 3;
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "definition_spelling": "3:6-3:9",
      "definition_extent": "3:1-5:2"
    }],
  "vars": [{
      "id": 0,
      "usr": "c:var_usage_static.cc@a",
      "short_name": "a",
      "detailed_name": "int a",
      "definition_spelling": "1:12-1:13",
      "definition_extent": "1:1-1:13",
      "is_local": false,
      "uses": ["1:12-1:13", "4:3-4:4"]
    }]
}
*/
