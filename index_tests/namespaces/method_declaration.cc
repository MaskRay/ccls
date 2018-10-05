namespace hello {
class Foo {
  void foo();
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
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["3:8-3:11|3:3-3:13|1025|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 2029211996748007610,
      "detailed_name": "namespace hello {}",
      "qual_name_offset": 10,
      "short_name": "hello",
      "bases": [],
      "funcs": [],
      "types": [4508214972876735896],
      "vars": [],
      "alias_of": 0,
      "kind": 3,
      "parent_kind": 0,
      "declarations": ["1:11-1:16|1:1-5:2|1|-1"],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 4508214972876735896,
      "detailed_name": "class hello::Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "spell": "2:7-2:10|2:1-4:2|1026|-1",
      "bases": [],
      "funcs": [10487325150128053272],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 3,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
