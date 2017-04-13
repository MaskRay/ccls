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
      "qualified_name": "accept",
      "declarations": ["2:6-2:12"],
      "callers": ["1@5:3-5:9", "1@6:3-6:9"]
    }, {
      "id": 1,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition_spelling": "4:6-4:9",
      "definition_extent": "4:1-7:2",
      "callees": ["0@5:3-5:9", "0@6:3-6:9"]
    }]
}
*/
