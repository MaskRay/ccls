#define MACRO_CALL(e) e

bool called(bool a, bool b);

void caller() {
  MACRO_CALL(called(true, true));
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@called#b#b#",
      "short_name": "called",
      "qualified_name": "called",
      "declarations": ["3:6-3:12"],
      "callers": ["1@6:14-6:20"],
      "uses": ["3:6-3:12", "6:14-6:20"]
    }, {
      "id": 1,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "qualified_name": "caller",
      "definition_spelling": "5:6-5:12",
      "definition_extent": "5:1-7:2",
      "callees": ["0@6:14-6:20"],
      "uses": ["5:6-5:12"]
    }]
}
*/