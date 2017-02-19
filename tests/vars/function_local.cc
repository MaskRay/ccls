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
      "short_name": "Foo",
      "qualified_name": "Foo",
      "declaration": "tests/vars/function_local.cc:1:8",
      "uses": ["tests/vars/function_local.cc:4:8"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/vars/function_local.cc:3:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:function_local.cc@31@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "declaration": "tests/vars/function_local.cc:4:8",
      "initializations": ["tests/vars/function_local.cc:4:8"],
      "variable_type": 0
    }]
}
*/