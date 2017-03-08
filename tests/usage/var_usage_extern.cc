extern int a;

void foo() {
  a = 5;
}
/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:3:6",
      "uses": ["1:3:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "declaration": "1:1:12",
      "uses": ["1:1:12", "1:4:3"]
    }]
}
*/
