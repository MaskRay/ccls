struct Wrapper {
  Wrapper(int i);
};

int called() { return 1; }

Wrapper caller() {
  return called();
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Wrapper",
      "short_name": "Wrapper",
      "detailed_name": "Wrapper",
      "definition_spelling": "1:8-1:15",
      "definition_extent": "1:1-3:2",
      "funcs": [0],
      "uses": ["1:8-1:15", "2:3-2:10", "7:1-7:8"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@S@Wrapper@F@Wrapper#I#",
      "short_name": "Wrapper",
      "detailed_name": "void Wrapper::Wrapper(int)",
      "declarations": ["2:3-2:10"],
      "declaring_type": 0,
      "callers": ["~2@8:10-8:16"]
    }, {
      "id": 1,
      "usr": "c:@F@called#",
      "short_name": "called",
      "detailed_name": "int called()",
      "definition_spelling": "5:5-5:11",
      "definition_extent": "5:1-5:27",
      "callers": ["2@8:10-8:16"]
    }, {
      "id": 2,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "detailed_name": "Wrapper caller()",
      "definition_spelling": "7:9-7:15",
      "definition_extent": "7:1-9:2",
      "callees": ["~0@8:10-8:16", "1@8:10-8:16"]
    }]
}
*/
