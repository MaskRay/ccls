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
      "uses": ["1:8-1:19", "*5:3-5:14"]
    }, {
      "id": 1,
      "usr": "c:@S@ImplementedType",
      "short_name": "ImplementedType",
      "qualified_name": "ImplementedType",
      "definition_spelling": "2:8-2:23",
      "definition_extent": "2:1-2:26",
      "instantiations": [1],
      "uses": ["*2:8-2:23", "*6:3-6:18"]
    }, {
      "id": 2,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition_spelling": "4:8-4:11",
      "definition_extent": "4:1-7:2",
      "vars": [0, 1],
      "uses": ["*4:8-4:11"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@a",
      "short_name": "a",
      "qualified_name": "Foo::a",
      "hover": "ForwardType *",
      "definition_spelling": "5:16-5:17",
      "definition_extent": "5:3-5:17",
      "variable_type": 0,
      "declaring_type": 2,
      "uses": ["5:16-5:17"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@b",
      "short_name": "b",
      "qualified_name": "Foo::b",
      "hover": "ImplementedType",
      "definition_spelling": "6:19-6:20",
      "definition_extent": "6:3-6:20",
      "variable_type": 1,
      "declaring_type": 2,
      "uses": ["6:19-6:20"]
    }]
}
*/
