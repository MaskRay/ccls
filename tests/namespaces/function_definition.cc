namespace hello {
void foo() {}
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@N@hello@F@foo#",
      "short_name": "foo",
      "qualified_name": "hello::foo",
      "hover": "void ()",
      "definition_spelling": "2:6-2:9",
      "definition_extent": "2:1-2:14"
    }]
}
*/
