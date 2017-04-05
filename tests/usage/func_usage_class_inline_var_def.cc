static int helper() {
  return 5;
}

class Foo {
  int x = helper();
};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition_spelling": "5:7-5:10",
      "definition_extent": "5:1-7:2",
      "vars": [0],
      "uses": ["*5:7-5:10"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:func_usage_class_inline_var_def.cc@F@helper#",
      "short_name": "helper",
      "qualified_name": "helper",
      "definition_spelling": "1:12-1:18",
      "definition_extent": "1:1-3:2",
      "uses": ["1:12-1:18", "6:11-6:17"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@x",
      "short_name": "x",
      "qualified_name": "Foo::x",
      "definition_spelling": "6:7-6:8",
      "definition_extent": "6:3-6:19",
      "declaring_type": 0,
      "uses": ["6:7-6:8"]
    }]
}
*/
