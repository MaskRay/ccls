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
  "types": [{
      "id": 0,
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
      "usr": 326583651986177228,
      "detailed_name": "int foo()",
      "short_name": "foo",
      "kind": 12,
      "storage": 3,
      "declarations": [],
      "spell": "2:12-2:15|-1|1|2",
      "extent": "2:1-4:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["6:9-6:12|-1|1|32", "7:9-7:12|-1|1|32"],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 16721564935990383768,
      "detailed_name": "int a",
      "short_name": "a",
      "hover": "int a = foo<int>()",
      "declarations": [],
      "spell": "6:5-6:6|-1|1|2",
      "extent": "6:1-6:19|-1|1|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 12028309045033782423,
      "detailed_name": "int b",
      "short_name": "b",
      "hover": "int b = foo<bool>()",
      "declarations": [],
      "spell": "7:5-7:6|-1|1|2",
      "extent": "7:1-7:20|-1|1|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
