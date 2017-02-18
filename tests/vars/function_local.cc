void foo() {
  int a;
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
      "definition": "tests/vars/function_local.cc:1:6",
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
      "short_name": "a",
      "qualified_name": "a",
      "declaration": "tests/vars/function_local.cc:2:7",
      "initializations": ["tests/vars/function_local.cc:2:7"],
      "variable_type": 0,
      "declaring_type": null,
      "uses": []
    }]
}
*/