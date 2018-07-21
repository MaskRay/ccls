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
      "kind": 9,
      "storage": 0,
      "declarations": [],
      "spell": "3:3-3:6|15041163540773201510|2|1026",
      "extent": "3:3-3:11|15041163540773201510|2|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["8:7-8:8|4259594751088586730|3|16676"],
      "callees": []
    }, {
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "7:6-7:9|0|1|2",
      "extent": "7:1-9:2|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [1893354193220338759],
      "uses": [],
      "callees": ["8:7-8:8|3385168158331140247|3|16676", "8:7-8:8|3385168158331140247|3|16676"]
    }, {
      "usr": 7440261702884428359,
      "detailed_name": "Foo::~Foo() noexcept",
      "qual_name_offset": 0,
      "short_name": "~Foo",
      "kind": 6,
      "storage": 0,
      "declarations": [],
      "spell": "4:3-4:4|15041163540773201510|2|1026",
      "extent": "4:3-4:12|15041163540773201510|2|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|0|1|2",
      "extent": "1:1-5:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [3385168158331140247, 7440261702884428359],
      "vars": [],
      "instances": [1893354193220338759],
      "uses": ["3:3-3:6|15041163540773201510|2|4", "4:4-4:7|15041163540773201510|2|4", "8:3-8:6|4259594751088586730|3|4"]
    }],
  "usr2var": [{
      "usr": 1893354193220338759,
      "detailed_name": "Foo f",
      "qual_name_offset": 4,
      "short_name": "f",
      "declarations": [],
      "spell": "8:7-8:8|4259594751088586730|3|2",
      "extent": "8:3-8:8|4259594751088586730|3|0",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
