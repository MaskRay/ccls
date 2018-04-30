template<typename T>
struct Foo {
  template<typename R>
  static int foo() {
    return 3;
  }
};

int a = Foo<int>::foo<float>();
int b = Foo<bool>::foo<double>();

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 9034026360701857235,
      "detailed_name": "int Foo::foo()",
      "qual_name_offset": 4,
      "short_name": "foo",
      "kind": 254,
      "storage": 3,
      "declarations": [],
      "spell": "4:14-4:17|10528472276654770367|2|2",
      "extent": "4:3-6:4|10528472276654770367|2|0",
      "declaring_type": 10528472276654770367,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["9:19-9:22|0|1|32", "10:20-10:23|0|1|32"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 17,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [16721564935990383768, 12028309045033782423],
      "uses": []
    }, {
      "usr": 10528472276654770367,
      "detailed_name": "Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "2:8-2:11|0|1|2",
      "extent": "2:1-7:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [9034026360701857235],
      "vars": [],
      "instances": [],
      "uses": ["9:9-9:12|0|1|4", "10:9-10:12|0|1|4"]
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int b = Foo<bool>::foo<double>()",
      "declarations": [],
      "spell": "10:5-10:6|0|1|2",
      "extent": "10:1-10:33|0|1|0",
      "type": 17,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "hover": "int a = Foo<int>::foo<float>()",
      "declarations": [],
      "spell": "9:5-9:6|0|1|2",
      "extent": "9:1-9:31|0|1|0",
      "type": 17,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
