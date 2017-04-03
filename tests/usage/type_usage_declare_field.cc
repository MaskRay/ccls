struct ForwardType;
struct ImplementedType {};

struct Foo {
  ForwardType* a;
  ImplementedType b;
};

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@ForwardType",
      "instantiations": [0],
      "uses": ["1:1:8", "*1:5:3"]
    }, {
      "id": 1,
      "usr": "c:@S@ImplementedType",
      "short_name": "ImplementedType",
      "qualified_name": "ImplementedType",
      "definition": "1:2:8",
      "instantiations": [1],
      "uses": ["*1:2:8", "*1:6:3"]
    }, {
      "id": 2,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "1:4:8",
      "vars": [0, 1],
      "uses": ["*1:4:8"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@a",
      "short_name": "a",
      "qualified_name": "Foo::a",
      "definition": "1:5:16",
      "variable_type": 0,
      "declaring_type": 2,
      "uses": ["1:5:16"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@b",
      "short_name": "b",
      "qualified_name": "Foo::b",
      "definition": "1:6:19",
      "variable_type": 1,
      "declaring_type": 2,
      "uses": ["1:6:19"]
    }]
}
*/
