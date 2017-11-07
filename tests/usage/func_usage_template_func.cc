template<typename T>
void accept(T);

void foo() {
  accept(1);
  accept(true);
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@FT@>1#Taccept#t0.0#v#",
      "short_name": "accept",
      "detailed_name": "void accept(T)",
      "is_constructor": false,
      "parameter_type_descriptions": ["T"],
      "declarations": [{
          "spelling": "2:6-2:12",
          "extent": "2:1-2:15",
          "content": "void accept(T)",
          "param_spellings": ["2:14-2:14"]
        }],
      "callers": ["1@5:3-5:9", "1@6:3-6:9"]
    }, {
      "id": 1,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "is_constructor": false,
      "definition_spelling": "4:6-4:9",
      "definition_extent": "4:1-7:2",
      "callees": ["0@5:3-5:9", "0@6:3-6:9"]
    }]
}
*/
