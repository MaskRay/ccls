class Foo {
public:
  Foo() {}
};

void foo() {
  Foo f;
  Foo* f2 = new Foo();
}

/*
// TODO: We should mark the constructor location inside of all_usages for the
//       type, so renames work. There's some code that sort of does this, but
//       it also includes implicit constructors. Maybe that's ok?
// TODO: check for implicit by comparing location to parent decl?
// TODO: At the moment, type rename is broken because we do not capture the
//      `new Foo()` as a reference to the Foo type.
//
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "*1:1:7",
      "funcs": [0],
      "all_uses": ["*1:1:7", "*1:7:3", "*1:8:3", "*1:8:17"],
      "interesting_uses": ["*1:7:3", "*1:8:3"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@Foo#",
      "short_name": "Foo",
      "qualified_name": "Foo::Foo",
      "definition": "*1:3:3",
      "declaring_type": 0,
      "callers": ["1@*1:7:7", "1@*1:8:17"],
      "all_uses": ["*1:3:3", "*1:7:7", "*1:8:17"]
    }, {
      "id": 1,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "*1:6:6",
      "callees": ["0@*1:7:7", "0@*1:8:17"],
      "all_uses": ["*1:6:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:constructor.cc@56@F@foo#@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition": "*1:7:7",
      "variable_type": 0,
      "all_uses": ["*1:7:7"]
    }, {
      "id": 1,
      "usr": "c:constructor.cc@66@F@foo#@f2",
      "short_name": "f2",
      "qualified_name": "f2",
      "definition": "*1:8:8",
      "variable_type": 0,
      "all_uses": ["*1:8:8"]
    }]
}
*/