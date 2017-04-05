struct Foo {};

template<class T>
Foo::Foo() {}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "1:8",
      "funcs": [0],
      "uses": ["*1:8", "4:6", "4:1"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foo@FT@>1#TFoo#v#",
      "short_name": "Foo",
      "qualified_name": "Foo::Foo",
      "definition": "4:6",
      "declaring_type": 0,
      "uses": ["4:6"]
    }]
}
*/
