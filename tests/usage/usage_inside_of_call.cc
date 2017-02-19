void called(int a);

int gen();

struct Foo {
  static int static_var;
  int field_var;
};

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
      "vars": [0, 1]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@called#I#",
      "short_name": "called",
      "qualified_name": "called",
      "declaration": "tests/usage/usage_inside_of_call.cc:1:6",
      "callers": ["2@tests/usage/usage_inside_of_call.cc:12:3"],
      "uses": ["tests/usage/usage_inside_of_call.cc:12:3"]
    }, {
      "id": 1,
      "usr": "c:@F@gen#",
      "short_name": "gen",
      "qualified_name": "gen",
      "declaration": "tests/usage/usage_inside_of_call.cc:3:5",
      "callers": ["2@tests/usage/usage_inside_of_call.cc:12:14"],
      "uses": ["tests/usage/usage_inside_of_call.cc:12:14"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/usage/usage_inside_of_call.cc:10:6",
      "callees": ["0@tests/usage/usage_inside_of_call.cc:12:3", "1@tests/usage/usage_inside_of_call.cc:12:14", "3@tests/usage/usage_inside_of_call.cc:12:22"]
    }, {
      "id": 3,
      "usr": "c:@S@Foo@F@Foo#",
      "callers": ["2@tests/usage/usage_inside_of_call.cc:12:22"],
      "uses": ["tests/usage/usage_inside_of_call.cc:12:22"]
    }],



  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@static_var",
      "short_name": "static_var",
      "qualified_name": "Foo::static_var",
      "declaration": "tests/usage/usage_inside_of_call.cc:6:14",
      "declaring_type": 0,
      "uses": ["tests/usage/usage_inside_of_call.cc:12:45"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@field_var",
      "short_name": "field_var",
      "qualified_name": "Foo::field_var",
      "declaration": "tests/usage/usage_inside_of_call.cc:7:7",
      "initializations": ["tests/usage/usage_inside_of_call.cc:7:7"],
      "declaring_type": 0,
      "uses": ["tests/usage/usage_inside_of_call.cc:12:28"]
    }, {
      "id": 2,
      "usr": "c:usage_inside_of_call.cc@117@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "declaration": "tests/usage/usage_inside_of_call.cc:11:7",
      "initializations": ["tests/usage/usage_inside_of_call.cc:11:7"],
      "uses": ["tests/usage/usage_inside_of_call.cc:12:10"]
    }]
}
*/