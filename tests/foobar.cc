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
      "detailed_name": "A",
      "definition_spelling": "1:6-1:7",
      "definition_extent": "1:1-1:10",
      "uses": ["1:6-1:7", "9:5-9:6"]
    }, {
      "id": 1,
      "usr": "c:@E@B",
      "short_name": "B",
      "detailed_name": "B",
      "definition_spelling": "2:6-2:7",
      "definition_extent": "2:1-2:10",
      "uses": ["2:6-2:7", "10:5-10:6"]
    }, {
      "id": 2,
      "usr": "c:@ST>1#T@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "5:8-5:11",
      "definition_extent": "5:1-7:2",
      "instances": [1],
      "uses": ["5:8-5:11", "9:1-9:4", "10:1-10:4"]
    }, {
      "id": 3,
      "usr": "c:@ST>1#T@Foo@S@Inner",
      "short_name": "Inner",
      "detailed_name": "Foo::Inner",
      "definition_spelling": "6:10-6:15",
      "definition_extent": "6:3-6:18",
      "instances": [0],
      "uses": ["6:10-6:15", "9:9-9:14"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "detailed_name": "Foo<A>::Inner a",
      "definition_spelling": "9:15-9:16",
      "definition_extent": "9:1-9:16",
      "variable_type": 3,
      "is_local": false,
      "uses": ["9:15-9:16"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "detailed_name": "Foo<B> b",
      "definition_spelling": "10:8-10:9",
      "definition_extent": "10:1-10:9",
      "variable_type": 2,
      "is_local": false,
      "uses": ["10:8-10:9"]
    }]
}
*/
