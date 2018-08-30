namespace hello {
void foo() {}
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 243328841292951622,
      "detailed_name": "void hello::foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "2:6-2:9|2029211996748007610|2|1026",
      "extent": "2:1-2:14|2029211996748007610|2|0",
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
      "declarations": ["1:11-1:16|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [243328841292951622],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
