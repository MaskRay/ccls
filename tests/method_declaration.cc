class Foo {
  void foo();
};

/*
// NOTE: Lack of declaring_type in functions and funcs in Foo is okay, because
//       those are processed when we find the definition for Foo::foo.
// TODO: Verify the strategy above works well with pure virtual interfaces and
//       the like (ie, we need to provide a good outline view). We could just
//       add the info in the declaration if the func is pure virtual - see
//       clang_CXXMethod_isPureVirtual

OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/method_declaration.cc:1:7",
      "all_uses": ["tests/method_declaration.cc:1:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "all_uses": ["tests/method_declaration.cc:2:8"]
    }],
  "variables": []
}
*/