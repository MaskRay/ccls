template<typename T>
void accept(T);

void foo() {
  accept(1);
  accept(true);
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@FT@>1#Taccept#t0.0#v#",
      "short_name": "accept",
      "detailed_name": "void accept(T)",
      "hover": "void accept(T)",
      "declarations": [{
          "spelling": "2:6-2:12",
          "extent": "2:1-2:15",
          "content": "void accept(T)",
          "param_spellings": ["2:14-2:14"]
        }],
      "derived": [],
      "locals": [],
      "callers": ["1@5:3-5:9", "1@6:3-6:9"],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "hover": "void foo()",
      "declarations": [],
      "definition_spelling": "4:6-4:9",
      "definition_extent": "4:1-7:2",
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["0@5:3-5:9", "0@6:3-6:9"]
    }],
  "vars": []
}
*/
