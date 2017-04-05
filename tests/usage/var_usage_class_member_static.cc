struct Foo {
  static int x;
};

void accept(int);

void foo() {
  accept(Foo::x);
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "1:8",
      "uses": ["*1:8", "8:10"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@accept#I#",
      "short_name": "accept",
      "qualified_name": "accept",
      "declarations": ["5:6"],
      "callers": ["1@8:3"],
      "uses": ["5:6", "8:3"]
    }, {
      "id": 1,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "7:6",
      "callees": ["0@8:3"],
      "uses": ["7:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@x",
      "short_name": "x",
      "qualified_name": "Foo::x",
      "declaration": "2:14",
      "uses": ["2:14", "8:15"]
    }]
}
*/
