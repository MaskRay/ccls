class Foo;
class Foo;
class Foo {};
class Foo;

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "short_name": "Foo",
      "qualified_name": "Foo",
      "declaration": "tests/class_forward_declaration.cc:1:7",
      "definition": "tests/class_forward_declaration.cc:3:7",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "uses": []
    }],
  "functions": [],
  "variables": []
}
*/