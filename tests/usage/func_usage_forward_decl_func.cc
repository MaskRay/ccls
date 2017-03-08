void foo();

void usage() {
  foo();
}
/*
OUTPUT:
{
  "types": [],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declarations": ["1:1:6"],
      "callers": ["1@1:4:3"],
      "uses": ["1:1:6", "1:4:3"]
    }, {
      "id": 1,
      "usr": "c:@F@usage#",
      "short_name": "usage",
      "qualified_name": "usage",
      "definition": "1:3:6",
      "callees": ["0@1:4:3"],
      "uses": ["1:3:6"]
    }]
}
*/
