template<typename T>
struct Foo {
  template<typename R>
  static int foo() {
    return 3;
  }
};

int a = Foo<int>::foo<float>();
int b = Foo<bool>::foo<double>();

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
      "uses": ["*2:8", "9:9", "10:9"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo@FT@>1#Tfoo#I#S",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "definition": "4:14",
      "declaring_type": 0,
      "uses": ["4:14", "9:19", "10:20"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "9:5",
      "uses": ["9:5"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition": "10:5",
      "uses": ["10:5"]
    }]
}
*/
