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
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@Wrapper",
      "short_name": "Wrapper",
      "detailed_name": "Wrapper",
      "definition_spelling": "1:8-1:15",
      "definition_extent": "1:1-3:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["1:8-1:15", "2:3-2:10", "7:1-7:8"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@S@Wrapper@F@Wrapper#I#",
      "short_name": "Wrapper",
      "detailed_name": "void Wrapper::Wrapper(int i)",
      "declarations": [{
          "spelling": "2:3-2:10",
          "extent": "2:3-2:17",
          "content": "Wrapper(int i)",
          "param_spellings": ["2:15-2:16"]
        }],
      "declaring_type": 0,
      "base": [],
      "derived": [],
      "locals": [],
      "callers": ["~2@8:10-8:16"],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@F@called#",
      "short_name": "called",
      "detailed_name": "int called()",
      "declarations": [],
      "definition_spelling": "5:5-5:11",
      "definition_extent": "5:1-5:27",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": ["2@8:10-8:16"],
      "callees": []
    }, {
      "id": 2,
      "is_operator": false,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "detailed_name": "Wrapper caller()",
      "declarations": [],
      "definition_spelling": "7:9-7:15",
      "definition_extent": "7:1-9:2",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["~0@8:10-8:16", "1@8:10-8:16"]
    }],
  "vars": []
}
*/
