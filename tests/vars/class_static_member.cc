class Foo {
  static int member;
};
int Foo::member = 0;
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/vars/class_static_member.cc:1:7",
      "vars": [0]
    }, {
      "id": 1,
      "uses": ["tests/vars/class_static_member.cc:2:14", "tests/vars/class_static_member.cc:4:10"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@member",
      "short_name": "member",
      "qualified_name": "Foo::member",
      "declaration": "tests/vars/class_static_member.cc:2:14",
      "initializations": ["tests/vars/class_static_member.cc:4:10"],
      "variable_type": 1,
      "declaring_type": 0
    }]
}
*/