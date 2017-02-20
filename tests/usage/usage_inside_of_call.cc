void called(int a);

int gen();

struct Foo {
  static int static_var;
  int field_var;
};

int Foo::static_var = 0;

void foo() {
  int a = 5;
  called(a + gen() + Foo().field_var + Foo::static_var);
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/usage/usage_inside_of_call.cc:5:8",
      "vars": [1, 0],
      "all_uses": ["tests/usage/usage_inside_of_call.cc:5:8", "tests/usage/usage_inside_of_call.cc:10:5", "tests/usage/usage_inside_of_call.cc:14:22", "tests/usage/usage_inside_of_call.cc:14:40"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@called#I#",
      "short_name": "called",
      "qualified_name": "called",
      "declaration": "tests/usage/usage_inside_of_call.cc:1:6",
      "callers": ["2@tests/usage/usage_inside_of_call.cc:14:3"],
      "all_uses": ["tests/usage/usage_inside_of_call.cc:1:6", "tests/usage/usage_inside_of_call.cc:14:3"]
    }, {
      "id": 1,
      "usr": "c:@F@gen#",
      "short_name": "gen",
      "qualified_name": "gen",
      "declaration": "tests/usage/usage_inside_of_call.cc:3:5",
      "callers": ["2@tests/usage/usage_inside_of_call.cc:14:14"],
      "all_uses": ["tests/usage/usage_inside_of_call.cc:3:5", "tests/usage/usage_inside_of_call.cc:14:14"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/usage/usage_inside_of_call.cc:12:6",
      "callees": ["0@tests/usage/usage_inside_of_call.cc:14:3", "1@tests/usage/usage_inside_of_call.cc:14:14"],
      "all_uses": ["tests/usage/usage_inside_of_call.cc:12:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@static_var",
      "short_name": "static_var",
      "qualified_name": "Foo::static_var",
      "declaration": "tests/usage/usage_inside_of_call.cc:6:14",
      "definition": "tests/usage/usage_inside_of_call.cc:10:10",
      "declaring_type": 0,
      "all_uses": ["tests/usage/usage_inside_of_call.cc:6:14", "tests/usage/usage_inside_of_call.cc:10:10", "tests/usage/usage_inside_of_call.cc:14:45"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@field_var",
      "short_name": "field_var",
      "qualified_name": "Foo::field_var",
      "definition": "tests/usage/usage_inside_of_call.cc:7:7",
      "declaring_type": 0,
      "all_uses": ["tests/usage/usage_inside_of_call.cc:7:7", "tests/usage/usage_inside_of_call.cc:14:28"]
    }, {
      "id": 2,
      "usr": "c:usage_inside_of_call.cc@145@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "tests/usage/usage_inside_of_call.cc:13:7",
      "all_uses": ["tests/usage/usage_inside_of_call.cc:13:7", "tests/usage/usage_inside_of_call.cc:14:10"]
    }]
}
*/