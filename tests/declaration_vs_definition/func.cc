void foo();
void foo();
void foo() {}
void foo();

/*
// Note: we always use the latest seen ("most local") definition/declaration.
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "declarations": [{
          "spelling": "1:6-1:9"
        }, {
          "spelling": "2:6-2:9"
        }, {
          "spelling": "4:6-4:9"
        }],
      "definition_spelling": "3:6-3:9",
      "definition_extent": "3:1-3:14"
    }]
}
*/
