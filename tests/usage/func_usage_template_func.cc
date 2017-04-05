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
      "declarations": ["2:6"],
      "callers": ["1@5:3", "1@6:3"],
      "uses": ["2:6", "5:3", "6:3"]
    }, {
      "id": 1,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "4:6",
      "callees": ["0@5:3", "0@6:3"],
      "uses": ["4:6"]
    }]
}
*/
