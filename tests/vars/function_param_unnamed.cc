void foo(int, int) {}
/*
// TODO: We should probably not emit variables for unnamed variables. But we
//       still need to emit reference information.
// TODO: This test is broken. Notice how we only emit one variable because
//       unnamed vars have no usr!

OUTPUT:
{
  "types": [{
      "id": 0
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#I#I#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/vars/function_param_unnamed.cc:1:6"
    }],
  "variables": [{
      "id": 0,
      "declaration": "tests/vars/function_param_unnamed.cc:1:13",
      "initializations": ["tests/vars/function_param_unnamed.cc:1:13", "tests/vars/function_param_unnamed.cc:1:18"],
      "variable_type": 0
    }]
}
*/