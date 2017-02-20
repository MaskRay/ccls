template<typename T>
void accept(T) {}

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
      "definition": "tests/usage/func_usage_template_func.cc:2:6",
      "callers": ["1@tests/usage/func_usage_template_func.cc:5:3", "1@tests/usage/func_usage_template_func.cc:6:3"],
      "all_uses": ["tests/usage/func_usage_template_func.cc:2:6", "tests/usage/func_usage_template_func.cc:5:3", "tests/usage/func_usage_template_func.cc:6:3"]
    }, {
      "id": 1,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/usage/func_usage_template_func.cc:4:6",
      "callees": ["0@tests/usage/func_usage_template_func.cc:5:3", "0@tests/usage/func_usage_template_func.cc:6:3"],
      "all_uses": ["tests/usage/func_usage_template_func.cc:4:6"]
    }],
  "variables": []
}
*/