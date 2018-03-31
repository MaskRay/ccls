namespace hello {
void foo() {}
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 2029211996748007610,
      "detailed_name": "hello",
      "short_name": "hello",
      "kind": 3,
      "declarations": [],
      "spell": "1:11-1:16|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [1],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["1:11-1:16|-1|1|4"]
    }, {
      "id": 1,
      "usr": 13838176792705659279,
      "detailed_name": "<fundamental>",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [0],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 243328841292951622,
      "detailed_name": "void hello::foo()",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "2:6-2:9|0|2|2",
      "extent": "2:1-2:14|0|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
*/
