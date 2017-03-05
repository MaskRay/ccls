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
      "definition": "1:2:8",
      "uses": ["*1:2:8", "1:6:9", "1:7:9"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo@var",
      "short_name": "var",
      "qualified_name": "Foo::var",
      "declaration": "1:3:24",
      "uses": ["1:3:24", "1:6:19", "1:7:20"]
    }, {
      "id": 1,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:6:5",
      "uses": ["1:6:5"]
    }, {
      "id": 2,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition": "1:7:5",
      "uses": ["1:7:5"]
    }]
}
*/