void foo(int, int) {}
/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#I#I#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/vars/function_param_unnamed.cc:1:6"
    }],
  "variables": []
}
*/