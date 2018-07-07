namespace hello {
class Foo {
  void foo() {}
};
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 10487325150128053272,
      "detailed_name": "void hello::Foo::foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 6,
      "storage": 0,
      "declarations": [],
      "spell": "3:8-3:11|4508214972876735896|2|514",
      "extent": "3:3-3:16|4508214972876735896|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 2029211996748007610,
      "detailed_name": "namespace hello {\n}",
      "qual_name_offset": 10,
      "short_name": "hello",
      "kind": 3,
      "declarations": ["1:11-1:16|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [4508214972876735896],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 4508214972876735896,
      "detailed_name": "class hello::Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:10|2029211996748007610|2|514",
      "extent": "2:1-4:2|2029211996748007610|2|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [10487325150128053272],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
