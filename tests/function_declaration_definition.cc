void foo();

void foo() {}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": "*1:1:6",
      "definition": "*1:3:6",
      "all_uses": ["*1:1:6", "*1:3:6"]
    }],
  "variables": []
}
*/