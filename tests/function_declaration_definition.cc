void foo();

void foo() {}

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
      "definition": "1:3:6",
      "uses": ["1:1:6", "1:3:6"]
    }]
}
*/
