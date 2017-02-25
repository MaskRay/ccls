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
      "definition": "1:5:8",
      "vars": [1, 0],
      "uses": ["*1:5:8", "1:10:5", "1:14:22", "1:14:40"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@called#I#",
      "short_name": "called",
      "qualified_name": "called",
      "declarations": ["1:1:6"],
      "callers": ["2@1:14:3"],
      "uses": ["1:1:6", "1:14:3"]
    }, {
      "id": 1,
      "usr": "c:@F@gen#",
      "short_name": "gen",
      "qualified_name": "gen",
      "declarations": ["1:3:5"],
      "callers": ["2@1:14:14"],
      "uses": ["1:3:5", "1:14:14"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:12:6",
      "callees": ["0@1:14:3", "1@1:14:14"],
      "uses": ["1:12:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@static_var",
      "short_name": "static_var",
      "qualified_name": "Foo::static_var",
      "declaration": "1:6:14",
      "definition": "1:10:10",
      "declaring_type": 0,
      "uses": ["1:6:14", "1:10:10", "1:14:45"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@field_var",
      "short_name": "field_var",
      "qualified_name": "Foo::field_var",
      "definition": "1:7:7",
      "declaring_type": 0,
      "uses": ["1:7:7", "1:14:28"]
    }, {
      "id": 2,
      "usr": "c:usage_inside_of_call.cc@145@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:13:7",
      "uses": ["1:13:7", "1:14:10"]
    }]
}
*/