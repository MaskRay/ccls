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
      "definition_spelling": "2:8-2:11",
      "definition_extent": "2:1-4:2",
      "uses": ["*2:8-2:11", "6:9-6:12", "7:9-7:12"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo@var",
      "short_name": "var",
      "qualified_name": "const int Foo::var",
      "declaration": "3:24-3:27",
      "uses": ["3:24-3:27", "6:19-6:22", "7:20-7:23"]
    }, {
      "id": 1,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "int a",
      "definition_spelling": "6:5-6:6",
      "definition_extent": "6:1-6:22",
      "uses": ["6:5-6:6"]
    }, {
      "id": 2,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "int b",
      "definition_spelling": "7:5-7:6",
      "definition_extent": "7:1-7:23",
      "uses": ["7:5-7:6"]
    }]
}
*/
