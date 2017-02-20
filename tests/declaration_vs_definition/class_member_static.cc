class Foo {
  static int foo;
};

int Foo::foo;

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/declaration_vs_definition/class_member_static.cc:1:7",
      "vars": [0],
      "all_uses": ["tests/declaration_vs_definition/class_member_static.cc:1:7", "tests/declaration_vs_definition/class_member_static.cc:5:5"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@foo",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "declaration": "tests/declaration_vs_definition/class_member_static.cc:2:14",
      "definition": "tests/declaration_vs_definition/class_member_static.cc:5:10",
      "declaring_type": 0,
      "all_uses": ["tests/declaration_vs_definition/class_member_static.cc:2:14", "tests/declaration_vs_definition/class_member_static.cc:5:10"]
    }]
}
*/