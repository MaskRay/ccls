class Foo {
public:
  Foo() {}
  ~Foo() {};
};

void foo() {
  Foo f;
}

// TODO: Support destructors (notice how the dtor has no usages listed).
//        - check if variable is a pointer. if so, do *not* insert dtor
//        - check if variable is normal type. if so, insert dtor
//        - scan for statements that look like dtors in function def handler
//        - figure out some way to support w/ unique_ptrs?
/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 3385168158331140247,
      "detailed_name": "Foo::Foo()",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "spell": "3:3-3:6|3:3-3:11|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 9,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["8:7-8:8|16676|-1"]
    }, {
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "spell": "7:6-7:9|7:1-9:2|2|-1",
      "bases": [],
      "vars": [1893354193220338759],
      "callees": ["8:7-8:8|3385168158331140247|3|16676", "8:7-8:8|3385168158331140247|3|16676"],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 7440261702884428359,
      "detailed_name": "Foo::~Foo() noexcept",
      "qual_name_offset": 0,
      "short_name": "~Foo",
      "spell": "4:3-4:7|4:3-4:12|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "spell": "1:7-1:10|1:1-5:2|2|-1",
      "bases": [],
      "funcs": [3385168158331140247, 7440261702884428359],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [1893354193220338759],
      "uses": ["3:3-3:6|4|-1", "4:4-4:7|4|-1", "8:3-8:6|4|-1"]
    }],
  "usr2var": [{
      "usr": 1893354193220338759,
      "detailed_name": "Foo f",
      "qual_name_offset": 4,
      "short_name": "f",
      "spell": "8:7-8:8|8:3-8:8|2|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
