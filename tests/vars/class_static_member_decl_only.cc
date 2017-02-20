class Foo {
  static int member;
};
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/vars/class_static_member_decl_only.cc:1:7",
      "all_uses": ["tests/vars/class_static_member_decl_only.cc:1:7"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@member",
      "short_name": "member",
      "qualified_name": "Foo::member",
      "declaration": "tests/vars/class_static_member_decl_only.cc:2:14",
      "all_uses": ["tests/vars/class_static_member_decl_only.cc:2:14"]
    }]
}
*/