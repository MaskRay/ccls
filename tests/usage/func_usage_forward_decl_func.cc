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
      "detailed_name": "void foo()",
      "declarations": [{
          "spelling": "1:6-1:9",
          "extent": "1:1-1:11",
          "content": "void foo()"
        }],
      "callers": ["1@4:3-4:6"]
    }, {
      "id": 1,
      "usr": "c:@F@usage#",
      "short_name": "usage",
      "detailed_name": "void usage()",
      "definition_spelling": "3:6-3:11",
      "definition_extent": "3:1-5:2",
      "callees": ["0@4:3-4:6"]
    }]
}
*/
