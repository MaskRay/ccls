namespace ns {
  template<typename T>
  struct Foo {
    template<typename R>
    static int foo() {
      return 3;
    }
  };

  int a = Foo<int>::foo<float>();
  int b = Foo<bool>::foo<double>();
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@N@ns@ST>1#T@Foo",
      "short_name": "Foo",
      "qualified_name": "ns::Foo",
      "definition": "3:10",
      "funcs": [0],
      "uses": ["*3:10", "10:11", "11:11"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@N@ns@ST>1#T@Foo@FT@>1#Tfoo#I#S",
      "short_name": "foo",
      "qualified_name": "ns::Foo::foo",
      "definition": "5:16",
      "declaring_type": 0,
      "uses": ["5:16", "10:21", "11:22"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@N@ns@a",
      "short_name": "a",
      "qualified_name": "ns::a",
      "definition": "10:7",
      "uses": ["10:7"]
    }, {
      "id": 1,
      "usr": "c:@N@ns@b",
      "short_name": "b",
      "qualified_name": "ns::b",
      "definition": "11:7",
      "uses": ["11:7"]
    }]
}
*/
