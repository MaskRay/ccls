namespace hello {
void foo() {}
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 243328841292951622,
      "detailed_name": "void hello::foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "2:6-2:9|2029211996748007610|2|2",
      "extent": "2:1-2:14|2029211996748007610|2|0",
      "declaring_type": 2029211996748007610,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 2029211996748007610,
      "detailed_name": "hello",
      "qual_name_offset": 0,
      "short_name": "hello",
      "kind": 3,
      "declarations": [],
      "spell": "1:11-1:16|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "alias_of": 0,
      "bases": [13838176792705659279],
      "derived": [],
      "types": [],
      "funcs": [243328841292951622],
      "vars": [],
      "instances": [],
      "uses": ["1:11-1:16|0|1|4"]
    }, {
      "usr": 13838176792705659279,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [2029211996748007610],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
