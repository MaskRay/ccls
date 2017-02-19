class Foo {
public:
  int x;
  int y;
};

void accept(int);
void accept(int*);

void foo() {
  Foo f;
  f.x = 3;
  f.x += 5;
  accept(f.x);
  accept(f.x + 20);
  accept(&f.x);
  accept(f.y);
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "tests/usage/var_usage_class_member.cc:1:7",
      "vars": [0, 1],
      "uses": ["tests/usage/var_usage_class_member.cc:11:7"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@accept#I#",
      "short_name": "accept",
      "qualified_name": "accept",
      "declaration": "tests/usage/var_usage_class_member.cc:7:6",
      "callers": ["2@tests/usage/var_usage_class_member.cc:14:3", "2@tests/usage/var_usage_class_member.cc:15:3", "2@tests/usage/var_usage_class_member.cc:17:3"],
      "uses": ["tests/usage/var_usage_class_member.cc:14:3", "tests/usage/var_usage_class_member.cc:15:3", "tests/usage/var_usage_class_member.cc:17:3"]
    }, {
      "id": 1,
      "usr": "c:@F@accept#*I#",
      "short_name": "accept",
      "qualified_name": "accept",
      "declaration": "tests/usage/var_usage_class_member.cc:8:6",
      "callers": ["2@tests/usage/var_usage_class_member.cc:16:3"],
      "uses": ["tests/usage/var_usage_class_member.cc:16:3"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "tests/usage/var_usage_class_member.cc:10:6",
      "callees": ["0@tests/usage/var_usage_class_member.cc:14:3", "0@tests/usage/var_usage_class_member.cc:15:3", "1@tests/usage/var_usage_class_member.cc:16:3", "0@tests/usage/var_usage_class_member.cc:17:3"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@x",
      "short_name": "x",
      "qualified_name": "Foo::x",
      "declaration": "tests/usage/var_usage_class_member.cc:3:7",
      "initializations": ["tests/usage/var_usage_class_member.cc:3:7"],
      "declaring_type": 0,
      "uses": ["tests/usage/var_usage_class_member.cc:12:5", "tests/usage/var_usage_class_member.cc:13:5", "tests/usage/var_usage_class_member.cc:14:12", "tests/usage/var_usage_class_member.cc:15:12", "tests/usage/var_usage_class_member.cc:16:13"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@y",
      "short_name": "y",
      "qualified_name": "Foo::y",
      "declaration": "tests/usage/var_usage_class_member.cc:4:7",
      "initializations": ["tests/usage/var_usage_class_member.cc:4:7"],
      "declaring_type": 0,
      "uses": ["tests/usage/var_usage_class_member.cc:17:12"]
    }, {
      "id": 2,
      "usr": "c:var_usage_class_member.cc@105@F@foo#@f",
      "short_name": "f",
      "qualified_name": "f",
      "declaration": "tests/usage/var_usage_class_member.cc:11:7",
      "initializations": ["tests/usage/var_usage_class_member.cc:11:7"],
      "variable_type": 0,
      "uses": ["tests/usage/var_usage_class_member.cc:12:3", "tests/usage/var_usage_class_member.cc:13:3", "tests/usage/var_usage_class_member.cc:14:10", "tests/usage/var_usage_class_member.cc:15:10", "tests/usage/var_usage_class_member.cc:16:11", "tests/usage/var_usage_class_member.cc:17:10"]
    }]
}
*/