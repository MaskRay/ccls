struct Foo;

void foo() {
  Foo* a;
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "all_uses": ["1:1:8", "*1:4:3"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:3:6",
      "all_uses": ["1:3:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:function_local.cc@31@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:4:8",
      "variable_type": 0,
      "all_uses": ["1:4:8"]
    }]
}
*/