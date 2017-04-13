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
      "definition_spelling": "1:8-1:11",
      "definition_extent": "1:1-3:2",
      "uses": ["*1:8-1:11", "8:10-8:13"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@accept#I#",
      "short_name": "accept",
      "qualified_name": "accept",
      "declarations": ["5:6-5:12"],
      "callers": ["1@8:3-8:9"]
    }, {
      "id": 1,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition_spelling": "7:6-7:9",
      "definition_extent": "7:1-9:2",
      "callees": ["0@8:3-8:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@x",
      "short_name": "x",
      "qualified_name": "Foo::x",
      "declaration": "2:14-2:15",
      "uses": ["2:14-2:15", "8:15-8:16"]
    }]
}
*/
