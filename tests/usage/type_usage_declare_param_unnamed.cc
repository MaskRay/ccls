struct ForwardType;
void foo(ForwardType*) {}
/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@ForwardType",
      "uses": ["1:8-1:19", "*2:10-2:21"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#*$@S@ForwardType#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition_spelling": "2:6-2:9",
      "definition_extent": "2:1-2:26",
      "uses": ["2:6-2:9"]
    }]
}
*/
