template<typename T>
struct Foo {
  static int foo() {
    return 3;
  }
};

int a = Foo<int>::foo();
int b = Foo<bool>::foo();

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
      "extent": "2:1-6:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["8:9-8:12|-1|1|4", "9:9-9:12|-1|1|4"]
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
      "instances": [0, 1],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 8340731781048851399,
      "detailed_name": "int Foo::foo()",
      "short_name": "foo",
      "kind": 254,
      "storage": 3,
      "declarations": [],
      "spell": "3:14-3:17|0|2|2",
      "extent": "3:3-5:4|0|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["8:19-8:22|-1|1|32", "9:20-9:23|-1|1|32"],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 16721564935990383768,
      "detailed_name": "int a",
      "short_name": "a",
      "hover": "int a = Foo<int>::foo()",
      "declarations": [],
      "spell": "8:5-8:6|-1|1|2",
      "extent": "8:1-8:24|-1|1|0",
      "type": 1,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 12028309045033782423,
      "detailed_name": "int b",
      "short_name": "b",
      "hover": "int b = Foo<bool>::foo()",
      "declarations": [],
      "spell": "9:5-9:6|-1|1|2",
      "extent": "9:1-9:25|-1|1|0",
      "type": 1,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
