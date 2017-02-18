void foo(int p) {
  int p = 0;
}
/*
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
      "definition": "tests/vars/function_shadow_param.cc:1:6",
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
      "short_name": "p",
      "qualified_name": "p",
      "declaration": "tests/vars/function_shadow_param.cc:1:14",
      "initializations": ["tests/vars/function_shadow_param.cc:1:14"],
      "variable_type": 0,
      "declaring_type": null,
      "uses": []
    }, {
      "id": 1,
      "short_name": "p",
      "qualified_name": "p",
      "declaration": "tests/vars/function_shadow_param.cc:2:7",
      "initializations": ["tests/vars/function_shadow_param.cc:2:7"],
      "variable_type": 0,
      "declaring_type": null,
      "uses": []
    }]
}
*/