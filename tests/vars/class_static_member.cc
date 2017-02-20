class Foo {
  static Foo* member;
};
Foo* Foo::member = nullptr;

/*
// TODO: Store both declaration and definition. It is very convenient to
//       quickly change between them.

OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/vars/class_static_member.cc:1:7",
      "vars": [0],
      "all_uses": ["tests/vars/class_static_member.cc:1:7", "tests/vars/class_static_member.cc:2:10", "tests/vars/class_static_member.cc:4:1", "tests/vars/class_static_member.cc:4:6"],
      "interesting_uses": ["tests/vars/class_static_member.cc:2:10", "tests/vars/class_static_member.cc:4:1"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@member",
      "short_name": "member",
      "qualified_name": "Foo::member",
      "declaration": "tests/vars/class_static_member.cc:4:11",
      "variable_type": 0,
      "declaring_type": 0,
      "all_uses": ["tests/vars/class_static_member.cc:2:15", "tests/vars/class_static_member.cc:4:11"]
    }]
}
*/