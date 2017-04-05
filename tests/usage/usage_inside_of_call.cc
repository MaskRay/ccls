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
      "definition": "5:8",
      "vars": [1, 0],
      "uses": ["*5:8", "10:5", "14:22", "14:40"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@called#I#",
      "short_name": "called",
      "qualified_name": "called",
      "declarations": ["1:6"],
      "callers": ["2@14:3"],
      "uses": ["1:6", "14:3"]
    }, {
      "id": 1,
      "usr": "c:@F@gen#",
      "short_name": "gen",
      "qualified_name": "gen",
      "declarations": ["3:5"],
      "callers": ["2@14:14"],
      "uses": ["3:5", "14:14"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "12:6",
      "callees": ["0@14:3", "1@14:14"],
      "uses": ["12:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@static_var",
      "short_name": "static_var",
      "qualified_name": "Foo::static_var",
      "declaration": "6:14",
      "definition": "10:10",
      "declaring_type": 0,
      "uses": ["6:14", "10:10", "14:45"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@field_var",
      "short_name": "field_var",
      "qualified_name": "Foo::field_var",
      "definition": "7:7",
      "declaring_type": 0,
      "uses": ["7:7", "14:28"]
    }, {
      "id": 2,
      "usr": "c:usage_inside_of_call.cc@145@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "13:7",
      "uses": ["13:7", "14:10"]
    }]
}
*/
