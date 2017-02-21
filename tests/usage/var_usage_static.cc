static int a;

void foo() {
  a = 3;
}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:3:6",
      "uses": ["1:3:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:var_usage_static.cc@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:1:12",
      "uses": ["1:1:12", "1:4:3"]
    }]
}
*/