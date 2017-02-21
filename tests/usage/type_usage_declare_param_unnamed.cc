struct ForwardType;
void foo(ForwardType*) {}
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@ForwardType",
      "all_uses": ["1:1:8", "*1:2:10"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@ForwardType#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:2:6",
      "all_uses": ["1:2:6"]
    }],
  "variables": []
}
*/