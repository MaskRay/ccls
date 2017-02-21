template<typename T>
void accept(T);

void foo() {
  accept(1);
  accept(true);
}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@FT@>1#Taccept#t0.0#v#",
      "short_name": "accept",
      "qualified_name": "accept",
      "declaration": "*1:2:6",
      "callers": ["1@*1:5:3", "1@*1:6:3"],
      "all_uses": ["*1:2:6", "*1:5:3", "*1:6:3"]
    }, {
      "id": 1,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "*1:4:6",
      "callees": ["0@*1:5:3", "0@*1:6:3"],
      "all_uses": ["*1:4:6"]
    }],
  "variables": []
}
*/