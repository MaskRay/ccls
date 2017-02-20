struct Foo {
  static int x;
};

void accept(int);

void foo() {
  accept(Foo::x);
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/usage/var_usage_class_member_static.cc:1:8",
      "all_uses": ["tests/usage/var_usage_class_member_static.cc:1:8", "tests/usage/var_usage_class_member_static.cc:8:10"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@accept#I#",
      "short_name": "accept",
      "qualified_name": "accept",
      "declaration": "tests/usage/var_usage_class_member_static.cc:5:6",
      "callers": ["1@tests/usage/var_usage_class_member_static.cc:8:3"],
      "all_uses": ["tests/usage/var_usage_class_member_static.cc:5:6", "tests/usage/var_usage_class_member_static.cc:8:3"]
    }, {
      "id": 1,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/usage/var_usage_class_member_static.cc:7:6",
      "callees": ["0@tests/usage/var_usage_class_member_static.cc:8:3"],
      "all_uses": ["tests/usage/var_usage_class_member_static.cc:7:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@x",
      "short_name": "x",
      "qualified_name": "Foo::x",
      "declaration": "tests/usage/var_usage_class_member_static.cc:2:14",
      "all_uses": ["tests/usage/var_usage_class_member_static.cc:2:14", "tests/usage/var_usage_class_member_static.cc:8:15"]
    }]
}
*/