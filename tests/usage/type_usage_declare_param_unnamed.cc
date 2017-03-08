struct ForwardType;
void foo(ForwardType*) {}
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@ForwardType",
      "uses": ["1:1:8", "*1:2:10"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@ForwardType#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:2:6",
      "uses": ["1:2:6"]
    }]
}
*/
