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
      "spell": "2:6-2:9|2:1-2:14|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 3,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 2029211996748007610,
      "detailed_name": "namespace hello {}",
      "qual_name_offset": 10,
      "short_name": "hello",
      "bases": [],
      "funcs": [243328841292951622],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 3,
      "parent_kind": 0,
      "declarations": ["1:11-1:16|1:1-3:2|1|-1"],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
