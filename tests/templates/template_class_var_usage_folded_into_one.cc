template<typename T>
struct Foo {
  static constexpr int var = 3;
};

int a = Foo<int>::var;
int b = Foo<bool>::var;

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "2:8",
      "uses": ["*2:8", "6:9", "7:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo@var",
      "short_name": "var",
      "qualified_name": "Foo::var",
      "declaration": "3:24",
      "uses": ["3:24", "6:19", "7:20"]
    }, {
      "id": 1,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "6:5",
      "uses": ["6:5"]
    }, {
      "id": 2,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition": "7:5",
      "uses": ["7:5"]
    }]
}
*/
