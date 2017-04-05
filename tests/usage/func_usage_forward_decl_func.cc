void foo();

void usage() {
  foo();
}
/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declarations": ["1:6"],
      "callers": ["1@4:3"],
      "uses": ["1:6", "4:3"]
    }, {
      "id": 1,
      "usr": "c:@F@usage#",
      "short_name": "usage",
      "qualified_name": "usage",
      "definition": "3:6",
      "callees": ["0@4:3"],
      "uses": ["3:6"]
    }]
}
*/
