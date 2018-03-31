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
      "usr": 10528472276654770367,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "2:8-2:11|-1|1|2",
      "extent": "2:1-4:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["6:9-6:12|-1|1|4", "7:9-7:12|-1|1|4"]
    }, {
      "id": 1,
      "usr": 17,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1, 2],
      "uses": []
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 13545144895171991916,
      "detailed_name": "const int Foo::var",
      "short_name": "var",
      "hover": "const int Foo::var = 3",
      "declarations": ["3:24-3:27|0|2|1"],
      "type": 1,
      "uses": ["6:19-6:22|-1|1|4", "7:20-7:23|-1|1|4"],
      "kind": 8,
      "storage": 3
    }, {
      "id": 1,
      "usr": 16721564935990383768,
      "detailed_name": "int a",
      "short_name": "a",
      "hover": "int a = Foo<int>::var",
      "declarations": [],
      "spell": "6:5-6:6|-1|1|2",
      "extent": "6:1-6:22|-1|1|0",
      "type": 1,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 2,
      "usr": 12028309045033782423,
      "detailed_name": "int b",
      "short_name": "b",
      "hover": "int b = Foo<bool>::var",
      "declarations": [],
      "spell": "7:5-7:6|-1|1|2",
      "extent": "7:1-7:23|-1|1|0",
      "type": 1,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
