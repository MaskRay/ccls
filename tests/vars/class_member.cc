class Foo {
  int member;
};
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "short_name": "Foo",
      "qualified_name": "Foo",
      "declaration": null,
      "definition": "tests/vars/class_member.cc:1:7",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0],
      "uses": []
    }, {
      "id": 1,
      "short_name": "",
      "qualified_name": "",
      "declaration": null
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "short_name": "member",
      "qualified_name": "Foo::member",
      "declaration": "tests/vars/class_member.cc:2:7",
      "initializations": ["tests/vars/class_member.cc:2:7"],
      "variable_type": 1,
      "declaring_type": 0,
      "uses": []
    }]
}
*/