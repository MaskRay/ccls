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
      "detailed_name": "ns::Foo",
      "definition_spelling": "3:10-3:13",
      "definition_extent": "3:3-8:4",
      "funcs": [0],
      "uses": ["3:10-3:13", "10:11-10:14", "11:11-11:14"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@N@ns@ST>1#T@Foo@FT@>1#Tfoo#I#S",
      "short_name": "foo",
      "detailed_name": "int ns::Foo::foo()",
      "definition_spelling": "5:16-5:19",
      "definition_extent": "5:5-7:6",
      "declaring_type": 0,
      "callers": ["-1@10:21-10:24", "-1@11:22-11:25"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@N@ns@a",
      "short_name": "a",
      "detailed_name": "int ns::a",
      "definition_spelling": "10:7-10:8",
      "definition_extent": "10:3-10:33",
      "is_local": false,
      "is_macro": false,
      "uses": ["10:7-10:8"]
    }, {
      "id": 1,
      "usr": "c:@N@ns@b",
      "short_name": "b",
      "detailed_name": "int ns::b",
      "definition_spelling": "11:7-11:8",
      "definition_extent": "11:3-11:35",
      "is_local": false,
      "is_macro": false,
      "uses": ["11:7-11:8"]
    }]
}
*/
