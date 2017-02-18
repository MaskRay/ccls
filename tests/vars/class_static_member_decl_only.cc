class Foo {
  static int member;
};
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "short_name": "Foo",
      "qualified_name": "Foo",
      "declaration": null,
      "definition": "tests/vars/class_static_member_decl_only.cc:1:7",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0],
      "uses": []
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "short_name": "member",
      "qualified_name": "Foo::member",
      "declaration": "tests/vars/class_static_member_decl_only.cc:2:14",
      "initializations": [],
      "variable_type": null,
      "declaring_type": 0,
      "uses": []
    }]
}
*/