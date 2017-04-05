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
      "qualified_name": "called",
      "declarations": ["1:6"],
      "callers": ["2@6:3"],
      "uses": ["1:6", "6:3"]
    }, {
      "id": 1,
      "usr": "c:@F@gen#",
      "short_name": "gen",
      "qualified_name": "gen",
      "definition": "3:5",
      "callers": ["2@6:10", "2@6:18"],
      "uses": ["3:5", "6:10", "6:18"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "5:6",
      "callees": ["0@6:3", "1@6:10", "1@6:18"],
      "uses": ["5:6"]
    }]
}
*/
