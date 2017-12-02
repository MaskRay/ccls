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
      "detailed_name": "Foo",
      "definition_spelling": "1:8-1:11",
      "definition_extent": "1:1-1:14",
      "funcs": [0],
      "uses": ["1:8-1:11", "4:6-4:9", "4:1-4:4"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@S@Foo@FT@>1#TFoo#v#",
      "short_name": "Foo",
      "detailed_name": "void Foo::Foo()",
      "definition_spelling": "4:6-4:9",
      "definition_extent": "4:1-4:11",
      "declaring_type": 0
    }]
}
*/
