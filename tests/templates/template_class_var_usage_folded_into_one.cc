template<typename T>
struct Foo {
  static constexpr int var = 3;
};

int a = Foo<int>::var;
int b = Foo<bool>::var;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "2:8-2:11",
      "definition_extent": "2:1-4:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:8-2:11", "6:9-6:12", "7:9-7:12"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": "c:@ST>1#T@Foo@var",
      "short_name": "var",
      "detailed_name": "const int Foo::var",
      "declaration": "3:24-3:27",
      "cls": 4,
      "uses": ["3:24-3:27", "6:19-6:22", "7:20-7:23"]
    }, {
      "id": 1,
      "usr": "c:@a",
      "short_name": "a",
      "detailed_name": "int a",
      "definition_spelling": "6:5-6:6",
      "definition_extent": "6:1-6:22",
      "cls": 3,
      "uses": ["6:5-6:6"]
    }, {
      "id": 2,
      "usr": "c:@b",
      "short_name": "b",
      "detailed_name": "int b",
      "definition_spelling": "7:5-7:6",
      "definition_extent": "7:1-7:23",
      "cls": 3,
      "uses": ["7:5-7:6"]
    }]
}
*/
