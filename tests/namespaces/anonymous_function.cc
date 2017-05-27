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
      "detailed_name": "void ::foo()",
      "declarations": [{
          "spelling": "2:6-2:9"
        }]
    }]
}
*/
