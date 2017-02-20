void called(int a);

int gen() { return 1; }

void foo() {
  called(gen() * gen());
}

/*
OUTPUT:
{
  "types": [],
  "functions": [{
      "id": 0,
      "usr": "c:@F@called#I#",
      "short_name": "called",
      "qualified_name": "called",
      "callers": ["2@tests/usage/usage_inside_of_call_simple.cc:6:3"],
      "all_uses": ["tests/usage/usage_inside_of_call_simple.cc:1:6", "tests/usage/usage_inside_of_call_simple.cc:6:3"]
    }, {
      "id": 1,
      "usr": "c:@F@gen#",
      "short_name": "gen",
      "qualified_name": "gen",
      "definition": "tests/usage/usage_inside_of_call_simple.cc:3:5",
      "callers": ["2@tests/usage/usage_inside_of_call_simple.cc:6:10", "2@tests/usage/usage_inside_of_call_simple.cc:6:18"],
      "all_uses": ["tests/usage/usage_inside_of_call_simple.cc:3:5", "tests/usage/usage_inside_of_call_simple.cc:6:10", "tests/usage/usage_inside_of_call_simple.cc:6:18"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/usage/usage_inside_of_call_simple.cc:5:6",
      "callees": ["0@tests/usage/usage_inside_of_call_simple.cc:6:3", "1@tests/usage/usage_inside_of_call_simple.cc:6:10", "1@tests/usage/usage_inside_of_call_simple.cc:6:18"],
      "all_uses": ["tests/usage/usage_inside_of_call_simple.cc:5:6"]
    }],
  "variables": []
}
*/