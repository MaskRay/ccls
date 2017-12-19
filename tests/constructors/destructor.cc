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
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "hover": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-5:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [0, 1],
      "vars": [],
      "instances": [0],
      "uses": ["1:7-1:10", "3:3-3:6", "4:4-4:7", "8:3-8:6"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@S@Foo@F@Foo#",
      "short_name": "Foo",
      "detailed_name": "void Foo::Foo()",
      "hover": "void Foo::Foo()",
      "declarations": [],
      "definition_spelling": "3:3-3:6",
      "definition_extent": "3:3-3:11",
      "declaring_type": 0,
      "base": [],
      "derived": [],
      "locals": [],
      "callers": ["~2@8:7-8:8"],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@S@Foo@F@~Foo#",
      "short_name": "~Foo",
      "detailed_name": "void Foo::~Foo() noexcept",
      "hover": "void Foo::~Foo() noexcept",
      "declarations": [],
      "definition_spelling": "4:3-4:7",
      "definition_extent": "4:3-4:12",
      "declaring_type": 0,
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }, {
      "id": 2,
      "is_operator": false,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "hover": "void foo()",
      "declarations": [],
      "definition_spelling": "7:6-7:9",
      "definition_extent": "7:1-9:2",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["~0@8:7-8:8"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:destructor.cc@70@F@foo#@f",
      "short_name": "f",
      "detailed_name": "Foo f",
      "hover": "Foo",
      "definition_spelling": "8:7-8:8",
      "definition_extent": "8:3-8:8",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "uses": ["8:7-8:8"]
    }]
}
*/
