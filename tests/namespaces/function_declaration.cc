namespace hello {
void foo(int a, int b);
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@N@hello@F@foo#I#I#",
      "short_name": "foo",
      "detailed_name": "void hello::foo(int, int)",
      "declarations": [{
          "spelling": "2:6-2:9"
        }]
    }]
}
*/
