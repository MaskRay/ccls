#define MACRO_CALL(e) e

bool called(bool a, bool b);

void caller() {
  MACRO_CALL(called(true, true));
}

/*
// TODO FIXME: called() has no callers.
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@called#b#b#",
      "short_name": "called",
      "qualified_name": "called",
      "declarations": ["3:6-3:12"],
      "uses": ["3:6-3:12"]
    }, {
      "id": 1,
      "usr": "c:@F@caller#",
      "short_name": "caller",
      "qualified_name": "caller",
      "definition_spelling": "5:6-5:12",
      "definition_extent": "5:1-7:2",
      "uses": ["5:6-5:12"]
    }]
}
*/