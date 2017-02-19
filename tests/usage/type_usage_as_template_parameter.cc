// TODO: Reenable
#if false
template<typename T>
class unique_ptr {
public:
  T value;
};

struct Foo {
  int x;
};

void foo() {
  unique_ptr<Foo> f0;
  unique_ptr<Foo> f1;

  f0.value.x += 5;
}
#endif
/*
// TODO: Figure out how we want to handle template specializations. For example,
//       when we use unique_ptr<int>.value, do we want to generalize that to a
//       usage on unique_ptr<T>.value, or just on unique_ptr<int>.value?

OUT2PUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#T@unique_ptr",
      "short_name": "unique_ptr",
      "qualified_name": "unique_ptr",
      "definition": "tests/usage/type_usage_as_template_parameter.cc:2:7",
      "vars": [0]
    }, {
      "id": 1,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/usage/type_usage_as_template_parameter.cc:7:8",
      "vars": [1],
      "uses": ["tests/usage/type_usage_as_template_parameter.cc:12:14", "tests/usage/type_usage_as_template_parameter.cc:13:14"]
    }, {
      "id": 2,
      "usr": "c:@S@unique_ptr>#$@S@Foo",
      "uses": ["tests/usage/type_usage_as_template_parameter.cc:12:19", "tests/usage/type_usage_as_template_parameter.cc:13:19"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/usage/type_usage_as_template_parameter.cc:11:6"
    }],
  "variables": [{
      "id": 0,
      "usr": "c:@ST>1#T@unique_ptr@FI@value",
      "short_name": "value",
      "qualified_name": "unique_ptr::value",
      "declaration": "tests/usage/type_usage_as_template_parameter.cc:4:5",
      "initializations": ["tests/usage/type_usage_as_template_parameter.cc:4:5"],
      "declaring_type": 0
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@x",
      "short_name": "x",
      "qualified_name": "Foo::x",
      "declaration": "tests/usage/type_usage_as_template_parameter.cc:8:7",
      "initializations": ["tests/usage/type_usage_as_template_parameter.cc:8:7"],
      "declaring_type": 1,
      "uses": ["tests/usage/type_usage_as_template_parameter.cc:15:12"]
    }, {
      "id": 2,
      "usr": "c:type_usage_as_template_parameter.cc@115@F@foo#@f0",
      "short_name": "f0",
      "qualified_name": "f0",
      "declaration": "tests/usage/type_usage_as_template_parameter.cc:12:19",
      "initializations": ["tests/usage/type_usage_as_template_parameter.cc:12:19"],
      "variable_type": 2,
      "uses": ["tests/usage/type_usage_as_template_parameter.cc:15:3"]
    }, {
      "id": 3,
      "usr": "c:type_usage_as_template_parameter.cc@138@F@foo#@f1",
      "short_name": "f1",
      "qualified_name": "f1",
      "declaration": "tests/usage/type_usage_as_template_parameter.cc:13:19",
      "initializations": ["tests/usage/type_usage_as_template_parameter.cc:13:19"],
      "variable_type": 2
    }, {
      "id": 4,
      "usr": "c:@S@unique_ptr>#$@S@Foo@FI@value",
      "uses": ["tests/usage/type_usage_as_template_parameter.cc:15:6"]
    }]
}

OUTPUT:
{
  "types": [],
  "functions": [],
  "variables": []
}

*/