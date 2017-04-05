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
      "declarations": ["1:6", "2:6", "4:6"],
      "definition": "3:6",
      "uses": ["1:6", "2:6", "3:6", "4:6"]
    }]
}

*/
