struct Foo {};

template<class T>
Foo::Foo() {}

/*
EXTRA_FLAGS:
-fms-compatibility
-fdelayed-template-parsing

OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 17319723337446061757,
      "detailed_name": "Foo::Foo::Foo()",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "spell": "4:6-4:9|4:1-4:11|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 9,
      "parent_kind": 23,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "1:8-1:11|1:1-1:14|2|-1",
      "bases": [],
      "funcs": [17319723337446061757],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["4:1-4:4|4|-1", "4:6-4:9|4|-1"]
    }],
  "usr2var": []
}
*/
