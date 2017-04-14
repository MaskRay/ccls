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
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-5:2",
      "funcs": [0, 1],
      "instantiations": [0],
      "uses": ["*1:7-1:10", "3:3-3:6", "4:3-4:7", "*8:3-8:6"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foo@F@Foo#",
      "short_name": "Foo",
      "qualified_name": "Foo::Foo",
      "hover": "void ()",
      "definition_spelling": "3:3-3:6",
      "definition_extent": "3:3-3:11",
      "declaring_type": 0,
      "callers": ["2@8:7-8:8"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@F@~Foo#",
      "short_name": "~Foo",
      "hover": "void () noexcept",
      "qualified_name": "Foo::~Foo",
      "definition_spelling": "4:3-4:7",
      "definition_extent": "4:3-4:12",
      "declaring_type": 0
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "hover": "void ()",
      "definition_spelling": "7:6-7:9",
      "definition_extent": "7:1-9:2",
      "callees": ["0@8:7-8:8"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:destructor.cc@70@F@foo#@f",
      "short_name": "f",
      "qualified_name": "f",
      "hover": "Foo",
      "definition_spelling": "8:7-8:8",
      "definition_extent": "8:3-8:8",
      "variable_type": 0,
      "uses": ["8:7-8:8"]
    }]
}
*/
