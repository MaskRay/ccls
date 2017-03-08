union Foo {
  int a : 5;
  bool b : 3;
};

Foo f;

void act(Foo*) {
  f.a = 3;
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@U@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "1:1:7",
      "vars": [0, 1],
      "uses": ["*1:1:7", "*1:6:1", "*1:8:10"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@act#*$@U@Foo#",
      "short_name": "act",
      "qualified_name": "act",
      "definition": "1:8:6",
      "uses": ["1:8:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@U@Foo@FI@a",
      "short_name": "a",
      "qualified_name": "Foo::a",
      "definition": "1:2:7",
      "declaring_type": 0,
      "uses": ["1:2:7", "1:9:5"]
    }, {
      "id": 1,
      "usr": "c:@U@Foo@FI@b",
      "short_name": "b",
      "qualified_name": "Foo::b",
      "definition": "1:3:8",
      "declaring_type": 0,
      "uses": ["1:3:8"]
    }, {
      "id": 2,
      "usr": "c:@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition": "1:6:5",
      "variable_type": 0,
      "uses": ["1:6:5", "1:9:3"]
    }]
}
*/
