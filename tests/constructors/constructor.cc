class Foo {
public:
  Foo() {}
};

void foo() {
  Foo f;
  Foo* f2 = new Foo();
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "1:1:7",
      "funcs": [0],
      "uses": ["*1:1:7", "1:3:3", "*1:7:3", "*1:8:3", "*1:8:17"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foo@F@Foo#",
      "short_name": "Foo",
      "qualified_name": "Foo::Foo",
      "definition": "1:3:3",
      "declaring_type": 0,
      "callers": ["1@1:7:7", "1@1:8:17"],
      "uses": ["1:3:3", "1:7:7", "1:8:17"]
    }, {
      "id": 1,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:6:6",
      "callees": ["0@1:7:7", "0@1:8:17"],
      "uses": ["1:6:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:constructor.cc@56@F@foo#@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition": "1:7:7",
      "variable_type": 0,
      "uses": ["1:7:7"]
    }, {
      "id": 1,
      "usr": "c:constructor.cc@66@F@foo#@f2",
      "short_name": "f2",
      "qualified_name": "f2",
      "definition": "1:8:8",
      "variable_type": 0,
      "uses": ["1:8:8"]
    }]
}
*/
