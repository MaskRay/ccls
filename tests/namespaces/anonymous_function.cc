namespace {
void foo();
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:anonymous_function.cc@aN@F@foo#",
      "short_name": "foo",
      "qualified_name": "::foo",
      "hover": "void ()",
      "declarations": ["2:6-2:9"]
    }]
}
*/
