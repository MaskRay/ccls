void foo(int, int) {}
/*
// TODO: We should probably not emit variables for unnamed variables. But we
//       still need to emit reference information.

OUTPUT:
{
  "types": [{
      "id": 0,
      "short_name": "",
      "qualified_name": "",
      "declaration": null
    }],
  "functions": [{
      "id": 0,
      "short_name": "foo",
      "qualified_name": "foo",
      "declaration": null,
      "definition": "tests/vars/function_param.cc:1:6",
      "declaring_type": null,
      "base": null,
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": [],
      "uses": []
    }],
  "variables": [{
      "id": 0,
      "short_name": "p0",
      "qualified_name": "p0",
      "declaration": "tests/vars/function_param.cc:1:14",
      "initializations": ["tests/vars/function_param.cc:1:14"],
      "variable_type": 0,
      "declaring_type": null,
      "uses": []
    }, {
      "id": 1,
      "short_name": "p1",
      "qualified_name": "p1",
      "declaration": "tests/vars/function_param.cc:1:22",
      "initializations": ["tests/vars/function_param.cc:1:22"],
      "variable_type": 0,
      "declaring_type": null,
      "uses": []
    }]
}
*/