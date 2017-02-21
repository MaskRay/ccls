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
      "definition": "1:1:7",
      "vars": [0, 1],
      "all_uses": ["1:1:7", "*1:11:3"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@accept#I#",
      "short_name": "accept",
      "qualified_name": "accept",
      "declaration": "1:7:6",
      "callers": ["2@1:14:3", "2@1:15:3", "2@1:17:3"],
      "all_uses": ["1:7:6", "1:14:3", "1:15:3", "1:17:3"]
    }, {
      "id": 1,
      "usr": "c:@F@accept#*I#",
      "short_name": "accept",
      "qualified_name": "accept",
      "declaration": "1:8:6",
      "callers": ["2@1:16:3"],
      "all_uses": ["1:8:6", "1:16:3"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "1:10:6",
      "callees": ["0@1:14:3", "0@1:15:3", "1@1:16:3", "0@1:17:3"],
      "all_uses": ["1:10:6"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@x",
      "short_name": "x",
      "qualified_name": "Foo::x",
      "definition": "1:3:7",
      "declaring_type": 0,
      "all_uses": ["1:3:7", "1:12:5", "1:13:5", "1:14:12", "1:15:12", "1:16:13"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@y",
      "short_name": "y",
      "qualified_name": "Foo::y",
      "definition": "1:4:7",
      "declaring_type": 0,
      "all_uses": ["1:4:7", "1:17:12"]
    }, {
      "id": 2,
      "usr": "c:var_usage_class_member.cc@105@F@foo#@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition": "1:11:7",
      "variable_type": 0,
      "all_uses": ["1:11:7", "1:12:3", "1:13:3", "1:14:10", "1:15:10", "1:16:11", "1:17:10"]
    }]
}
*/