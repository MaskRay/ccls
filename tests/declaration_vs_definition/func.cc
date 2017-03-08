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
      "qualified_name": "foo",
      "declarations": ["1:1:6", "1:2:6", "1:4:6"],
      "definition": "1:3:6",
      "uses": ["1:1:6", "1:2:6", "1:3:6", "1:4:6"]
    }]
}

*/
