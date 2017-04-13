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
      "definition_spelling": "5:8-5:11",
      "definition_extent": "5:1-8:2",
      "vars": [1, 0],
      "uses": ["*5:8-5:11", "10:5-10:8", "14:22-14:25", "14:40-14:43"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@called#I#",
      "short_name": "called",
      "qualified_name": "called",
      "declarations": ["1:6-1:12"],
      "callers": ["2@14:3-14:9"]
    }, {
      "id": 1,
      "usr": "c:@F@gen#",
      "short_name": "gen",
      "qualified_name": "gen",
      "declarations": ["3:5-3:8"],
      "callers": ["2@14:14-14:17"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition_spelling": "12:6-12:9",
      "definition_extent": "12:1-15:2",
      "callees": ["0@14:3-14:9", "1@14:14-14:17"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@static_var",
      "short_name": "static_var",
      "qualified_name": "Foo::static_var",
      "declaration": "6:14-6:24",
      "definition_spelling": "10:10-10:20",
      "definition_extent": "10:1-10:24",
      "declaring_type": 0,
      "uses": ["6:14-6:24", "10:10-10:20", "14:45-14:55"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@field_var",
      "short_name": "field_var",
      "qualified_name": "Foo::field_var",
      "definition_spelling": "7:7-7:16",
      "definition_extent": "7:3-7:16",
      "declaring_type": 0,
      "uses": ["7:7-7:16", "14:28-14:37"]
    }, {
      "id": 2,
      "usr": "c:usage_inside_of_call.cc@145@F@foo#@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition_spelling": "13:7-13:8",
      "definition_extent": "13:3-13:12",
      "uses": ["13:7-13:8", "14:10-14:11"]
    }]
}
*/
