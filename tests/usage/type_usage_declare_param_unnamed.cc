struct ForwardType;
void foo(ForwardType*) {}
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@ForwardType",
      "uses": ["1:8-1:19", "2:10-2:21"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@ForwardType#",
      "short_name": "foo",
      "detailed_name": "void foo(ForwardType *)",
      "definition_spelling": "2:6-2:9",
      "definition_extent": "2:1-2:26"
    }]
}
*/
