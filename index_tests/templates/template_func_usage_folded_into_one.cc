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
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 326583651986177228,
      "detailed_name": "int foo()",
      "qual_name_offset": 4,
      "short_name": "foo",
      "kind": 12,
      "storage": 3,
      "declarations": [],
      "spell": "2:12-2:15|0|1|2",
      "extent": "2:1-4:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:9-6:12|0|1|32", "7:9-7:12|0|1|32"],
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
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int b = foo<bool>()",
      "declarations": [],
      "spell": "7:5-7:6|0|1|2",
      "extent": "7:1-7:20|0|1|0",
      "type": 17,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "hover": "int a = foo<int>()",
      "declarations": [],
      "spell": "6:5-6:6|0|1|2",
      "extent": "6:1-6:19|0|1|0",
      "type": 17,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
