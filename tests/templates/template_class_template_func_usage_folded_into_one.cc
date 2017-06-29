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
      "detailed_name": "Foo",
      "definition_spelling": "2:8-2:11",
      "definition_extent": "2:1-7:2",
      "funcs": [0],
      "uses": ["2:8-2:11", "9:9-9:12", "10:9-10:12"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo@FT@>1#Tfoo#I#S",
      "short_name": "foo",
      "detailed_name": "int Foo::foo()",
      "definition_spelling": "4:14-4:17",
      "definition_extent": "4:3-6:4",
      "declaring_type": 0,
      "callers": ["-1@9:19-9:22", "-1@10:20-10:23"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "detailed_name": "int a",
      "definition_spelling": "9:5-9:6",
      "definition_extent": "9:1-9:31",
      "is_local": false,
      "is_macro": false,
      "uses": ["9:5-9:6"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "detailed_name": "int b",
      "definition_spelling": "10:5-10:6",
      "definition_extent": "10:1-10:33",
      "is_local": false,
      "is_macro": false,
      "uses": ["10:5-10:6"]
    }]
}
*/
