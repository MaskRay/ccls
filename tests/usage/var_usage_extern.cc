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
      "hover": "void ()",
      "definition_spelling": "3:6-3:9",
      "definition_extent": "3:1-5:2"
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "hover": "int",
      "declaration": "1:12-1:13",
      "uses": ["1:12-1:13", "4:3-4:4"]
    }]
}
*/
