void called(int a);

int gen() { return 1; }

void foo() {
  called(gen() * gen());
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@called#I#",
      "short_name": "called",
      "detailed_name": "void called(int)",
      "declarations": ["1:6-1:12"],
      "callers": ["2@6:3-6:9"]
    }, {
      "id": 1,
      "usr": "c:@F@gen#",
      "short_name": "gen",
      "detailed_name": "int gen()",
      "definition_spelling": "3:5-3:8",
      "definition_extent": "3:1-3:24",
      "callers": ["2@6:10-6:13", "2@6:18-6:21"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "definition_spelling": "5:6-5:9",
      "definition_extent": "5:1-7:2",
      "callees": ["0@6:3-6:9", "1@6:10-6:13", "1@6:18-6:21"]
    }]
}
*/
