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
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 5,
      "declarations": ["3:3-3:6|-1|1|4", "4:4-4:7|-1|1|4"],
      "spell": "1:7-1:10|-1|1|2",
      "extent": "1:1-5:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0, 1],
      "vars": [],
      "instances": [0],
      "uses": ["3:3-3:6|0|2|4", "8:3-8:6|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 3385168158331140247,
      "detailed_name": "void Foo::Foo()",
      "short_name": "Foo",
      "kind": 9,
      "storage": 1,
      "declarations": [],
      "spell": "3:3-3:6|0|2|2",
      "extent": "3:3-3:11|0|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": ["8:7-8:8|2|3|288"],
      "callees": []
    }, {
      "id": 1,
      "usr": 7440261702884428359,
      "detailed_name": "void Foo::~Foo() noexcept",
      "short_name": "~Foo",
      "kind": 6,
      "storage": 1,
      "declarations": [],
      "spell": "4:3-4:7|0|2|2",
      "extent": "4:3-4:12|0|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 2,
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "7:6-7:9|-1|1|2",
      "extent": "7:1-9:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [0],
      "uses": [],
      "callees": ["8:7-8:8|0|3|288", "8:7-8:8|0|3|288"]
    }],
  "vars": [{
      "id": 0,
      "usr": 9954632887635271906,
      "detailed_name": "Foo f",
      "short_name": "f",
      "declarations": [],
      "spell": "8:7-8:8|2|3|2",
      "extent": "8:3-8:8|2|3|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
