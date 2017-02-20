class Foo {
public:
  Foo() {}
};

void foo() {
  Foo f;
  Foo* f2 = new Foo();
}

/*
// TODO: We should mark the constructor location inside of all_usages for the type, so renames work.
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/constructors/constructor.cc:1:7",
      "funcs": [0],
      "all_uses": ["tests/constructors/constructor.cc:1:7", "tests/constructors/constructor.cc:7:3", "tests/constructors/constructor.cc:8:3", "tests/constructors/constructor.cc:8:17"],
      "interesting_uses": ["tests/constructors/constructor.cc:7:3", "tests/constructors/constructor.cc:8:3"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@Foo#",
      "short_name": "Foo",
      "qualified_name": "Foo::Foo",
      "definition": "tests/constructors/constructor.cc:3:3",
      "declaring_type": 0,
      "callers": ["1@tests/constructors/constructor.cc:7:7", "1@tests/constructors/constructor.cc:8:17"],
      "all_uses": ["tests/constructors/constructor.cc:3:3", "tests/constructors/constructor.cc:7:7", "tests/constructors/constructor.cc:8:17"]
    }, {
      "id": 1,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/constructors/constructor.cc:6:6",
      "callees": ["0@tests/constructors/constructor.cc:7:7", "0@tests/constructors/constructor.cc:8:17"],
      "all_uses": ["tests/constructors/constructor.cc:6:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:constructor.cc@56@F@foo#@f",
      "short_name": "f",
      "qualified_name": "f",
      "declaration": "tests/constructors/constructor.cc:7:7",
      "variable_type": 0,
      "all_uses": ["tests/constructors/constructor.cc:7:7"]
    }, {
      "id": 1,
      "usr": "c:constructor.cc@66@F@foo#@f2",
      "short_name": "f2",
      "qualified_name": "f2",
      "declaration": "tests/constructors/constructor.cc:8:8",
      "variable_type": 0,
      "all_uses": ["tests/constructors/constructor.cc:8:8"]
    }]
}
*/