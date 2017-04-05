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
      "definition_spelling": "2:8-2:11",
      "definition_extent": "2:1-6:2",
      "funcs": [0],
      "uses": ["*2:8-2:11", "8:9-8:12", "9:9-9:12"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo@F@foo#S",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "definition_spelling": "3:14-3:17",
      "definition_extent": "3:3-5:4",
      "declaring_type": 0,
      "uses": ["3:14-3:17", "8:19-8:22", "9:20-9:23"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition_spelling": "8:5-8:6",
      "definition_extent": "8:1-8:24",
      "uses": ["8:5-8:6"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition_spelling": "9:5-9:6",
      "definition_extent": "9:1-9:25",
      "uses": ["9:5-9:6"]
    }]
}
*/
