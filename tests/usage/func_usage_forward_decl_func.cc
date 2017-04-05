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
      "declarations": ["1:6-1:9"],
      "callers": ["1@4:3-4:6"],
      "uses": ["1:6-1:9", "4:3-4:6"]
    }, {
      "id": 1,
      "usr": "c:@F@usage#",
      "short_name": "usage",
      "qualified_name": "usage",
      "definition_spelling": "3:6-3:11",
      "definition_extent": "3:1-5:2",
      "callees": ["0@4:3-4:6"],
      "uses": ["3:6-3:11"]
    }]
}
*/
