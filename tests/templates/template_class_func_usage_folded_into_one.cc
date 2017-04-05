template<typename T>
struct Foo {
  static int foo() {
    return 3;
  }
};

int a = Foo<int>::foo();
int b = Foo<bool>::foo();

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "2:8",
      "funcs": [0],
      "uses": ["*2:8", "8:9", "9:9"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo@F@foo#S",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "definition": "3:14",
      "declaring_type": 0,
      "uses": ["3:14", "8:19", "9:20"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "8:5",
      "uses": ["8:5"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition": "9:5",
      "uses": ["9:5"]
    }]
}
*/
