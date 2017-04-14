void called();

template <typename T>
void caller() {
  called();
}

void foo() {
  caller<int>();
}

/*
// NOTE: without caller<int>() instantation caller() is never visited so
// called() is never referenced.
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@called#",
      "short_name": "called",
      "qualified_name": "called",
      "hover": "void ()",
      "declarations": ["1:6-1:12"],
      "callers": ["1@5:3-5:9"]
    }, {
      "id": 1,
      "usr": "c:@FT@>1#Tcaller#v#",
      "short_name": "caller",
      "qualified_name": "caller",
      "hover": "void ()",
      "definition_spelling": "4:6-4:12",
      "definition_extent": "4:1-6:2",
      "callers": ["2@9:3-9:9"],
      "callees": ["0@5:3-5:9"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "hover": "void ()",
      "definition_spelling": "8:6-8:9",
      "definition_extent": "8:1-10:2",
      "callees": ["1@9:3-9:9"]
    }]
}
*/