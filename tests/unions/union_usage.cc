union Foo {
  int a : 5;
  bool b : 3;
};

Foo f;

void act(Foo*) {
  f.a = 3;
}

/*
// TODO: instantiations on Foo should include parameter?

OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@U@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "1:7",
      "vars": [0, 1],
      "instantiations": [2],
      "uses": ["*1:7", "*6:1", "*8:10"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@act#*$@U@Foo#",
      "short_name": "act",
      "qualified_name": "act",
      "definition": "8:6",
      "uses": ["8:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@U@Foo@FI@a",
      "short_name": "a",
      "qualified_name": "Foo::a",
      "definition": "2:7",
      "declaring_type": 0,
      "uses": ["2:7", "9:5"]
    }, {
      "id": 1,
      "usr": "c:@U@Foo@FI@b",
      "short_name": "b",
      "qualified_name": "Foo::b",
      "definition": "3:8",
      "declaring_type": 0,
      "uses": ["3:8"]
    }, {
      "id": 2,
      "usr": "c:@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition": "6:5",
      "variable_type": 0,
      "uses": ["6:5", "9:3"]
    }]
}
*/
