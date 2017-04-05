void foo();

void foo() {}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declarations": ["1:6"],
      "definition": "3:6",
      "uses": ["1:6", "3:6"]
    }]
}
*/
