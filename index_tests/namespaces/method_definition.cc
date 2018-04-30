namespace hello {
class Foo {
  void foo();
};

void Foo::foo() {}
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [{
      "usr": 10487325150128053272,
      "detailed_name": "void hello::Foo::foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 6,
      "storage": 1,
      "declarations": ["3:8-3:11|4508214972876735896|2|1"],
      "spell": "6:11-6:14|4508214972876735896|2|2",
      "extent": "6:1-6:19|2029211996748007610|2|0",
      "declaring_type": 4508214972876735896,
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
      "extent": "1:1-7:2|0|1|0",
      "alias_of": 0,
      "bases": [13838176792705659279],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["1:11-1:16|0|1|4"]
    }, {
      "usr": 4508214972876735896,
      "detailed_name": "hello::Foo",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:10|2029211996748007610|2|2",
      "extent": "2:1-4:2|2029211996748007610|2|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [10487325150128053272],
      "vars": [],
      "instances": [],
      "uses": ["6:6-6:9|0|1|4"]
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
