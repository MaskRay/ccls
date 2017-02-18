namespace hello {
void foo(int a, int b);
}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "short_name": "foo",
      "qualified_name": "hello::foo",
      "declaration": "tests/function_declaration_in_namespace.cc:2:6"
    }],
  "variables": []
}
*/