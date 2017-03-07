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
      "definition": "1:2:8",
      "funcs": [0],
      "uses": ["*1:2:8", "1:8:9", "1:9:9"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo@F@foo#S",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "definition": "1:3:14",
      "declaring_type": 0,
      "uses": ["1:3:14", "1:8:19", "1:9:20"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:8:5",
      "uses": ["1:8:5"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition": "1:9:5",
      "uses": ["1:9:5"]
    }]
}
*/
