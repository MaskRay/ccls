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
      "definition": "5:7",
      "vars": [0],
      "uses": ["*5:7"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:func_usage_class_inline_var_def.cc@F@helper#",
      "short_name": "helper",
      "qualified_name": "helper",
      "definition": "1:12",
      "uses": ["1:12", "6:11"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@x",
      "short_name": "x",
      "qualified_name": "Foo::x",
      "definition": "6:7",
      "declaring_type": 0,
      "uses": ["6:7"]
    }]
}
*/
