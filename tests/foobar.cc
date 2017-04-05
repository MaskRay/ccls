enum A {};
enum B {};

template<typename T>
struct Foo {
  struct Inner {};
};

Foo<A>::Inner a;
Foo<B> b;
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@E@A",
      "short_name": "A",
      "qualified_name": "A",
      "definition": "1:6",
      "uses": ["*1:6", "*9:5"]
    }, {
      "id": 1,
      "usr": "c:@E@B",
      "short_name": "B",
      "qualified_name": "B",
      "definition": "2:6",
      "uses": ["*2:6", "*10:5"]
    }, {
      "id": 2,
      "usr": "c:@ST>1#T@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "5:8",
      "instantiations": [1],
      "uses": ["*5:8", "*9:1", "*10:1"]
    }, {
      "id": 3,
      "usr": "c:@ST>1#T@Foo@S@Inner",
      "short_name": "Inner",
      "qualified_name": "Foo::Inner",
      "definition": "6:10",
      "instantiations": [0],
      "uses": ["*6:10", "*9:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "9:15",
      "variable_type": 3,
      "uses": ["9:15"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition": "10:8",
      "variable_type": 2,
      "uses": ["10:8"]
    }]
}

*/
