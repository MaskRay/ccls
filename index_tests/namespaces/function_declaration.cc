namespace hello {
void foo(int a, int b);
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 18343102288837190527,
      "detailed_name": "void hello::foo(int a, int b)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": ["2:6-2:9|2:1-2:23|2029211996748007610|2|1025|-1"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 2029211996748007610,
      "detailed_name": "namespace hello {}",
      "qual_name_offset": 10,
      "short_name": "hello",
      "kind": 3,
      "declarations": ["1:11-1:16|1:1-3:2|0|1|1|-1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [18343102288837190527],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
