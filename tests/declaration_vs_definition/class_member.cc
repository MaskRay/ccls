class Foo {
  int foo;
};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/declaration_vs_definition/class_member.cc:1:7",
      "vars": [0],
      "all_uses": ["tests/declaration_vs_definition/class_member.cc:1:7"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@foo",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "definition": "tests/declaration_vs_definition/class_member.cc:2:7",
      "declaring_type": 0,
      "all_uses": ["tests/declaration_vs_definition/class_member.cc:2:7"]
    }]
}
*/