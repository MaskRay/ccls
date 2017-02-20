struct Foo;

void foo() {
  Foo* a;
}
/*
// TODO: Make sure usage for Foo is inserted into type section.

OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "all_uses": ["tests/vars/function_local.cc:1:8", "tests/vars/function_local.cc:4:3"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/vars/function_local.cc:3:6",
      "all_uses": ["tests/vars/function_local.cc:3:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:function_local.cc@31@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "declaration": "tests/vars/function_local.cc:4:8",
      "variable_type": 0,
      "all_uses": ["tests/vars/function_local.cc:4:8"]
    }]
}
*/