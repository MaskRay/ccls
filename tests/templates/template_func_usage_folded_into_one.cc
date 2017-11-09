template<typename T>
static int foo() {
  return 3;
}

int a = foo<int>();
int b = foo<bool>();

// TODO: put template foo inside a namespace
// TODO: put template foo inside a template class inside a namespace

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:template_func_usage_folded_into_one.cc@FT@>1#Tfoo#I#",
      "short_name": "foo",
      "detailed_name": "int foo()",
      "definition_spelling": "2:12-2:15",
      "definition_extent": "2:1-4:2",
      "callers": ["-1@6:9-6:12", "-1@7:9-7:12"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "detailed_name": "int a",
      "definition_spelling": "6:5-6:6",
      "definition_extent": "6:1-6:19",
      "is_local": false,
      "is_macro": false,
      "uses": ["6:5-6:6"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "detailed_name": "int b",
      "definition_spelling": "7:5-7:6",
      "definition_extent": "7:1-7:20",
      "is_local": false,
      "is_macro": false,
      "uses": ["7:5-7:6"]
    }]
}
*/
