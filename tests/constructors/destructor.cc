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
      "definition": "1:7",
      "funcs": [0, 1],
      "instantiations": [0],
      "uses": ["*1:7", "3:3", "4:3", "*8:3"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Foo@F@Foo#",
      "short_name": "Foo",
      "qualified_name": "Foo::Foo",
      "definition": "3:3",
      "declaring_type": 0,
      "callers": ["2@8:7"],
      "uses": ["3:3", "8:7"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@F@~Foo#",
      "short_name": "~Foo",
      "qualified_name": "Foo::~Foo",
      "definition": "4:3",
      "declaring_type": 0,
      "uses": ["4:3"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "7:6",
      "callees": ["0@8:7"],
      "uses": ["7:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:destructor.cc@70@F@foo#@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition": "8:7",
      "variable_type": 0,
      "uses": ["8:7"]
    }]
}
*/
