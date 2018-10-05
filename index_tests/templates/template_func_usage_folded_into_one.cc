template<typename T>
static int foo() {
  return 3;
}

int a = foo<int>();
int b = foo<bool>();

// TODO: put template foo inside a namespace
// TODO: put template foo inside a template class inside a namespace

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 326583651986177228,
      "detailed_name": "static int foo()",
      "qual_name_offset": 11,
      "short_name": "foo",
      "spell": "2:12-2:15|2:1-4:2|2|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["6:9-6:12|36|-1", "7:9-7:12|36|-1"]
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [16721564935990383768, 12028309045033782423],
      "uses": []
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int b = foo<bool>()",
      "spell": "7:5-7:6|7:1-7:20|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "hover": "int a = foo<int>()",
      "spell": "6:5-6:6|6:1-6:19|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
