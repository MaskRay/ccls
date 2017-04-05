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
      "definition": "1:7",
      "vars": [0, 1],
      "instantiations": [2],
      "uses": ["*1:7", "*11:3"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@accept#I#",
      "short_name": "accept",
      "qualified_name": "accept",
      "declarations": ["7:6"],
      "callers": ["2@14:3", "2@15:3", "2@17:3"],
      "uses": ["7:6", "14:3", "15:3", "17:3"]
    }, {
      "id": 1,
      "usr": "c:@F@accept#*I#",
      "short_name": "accept",
      "qualified_name": "accept",
      "declarations": ["8:6"],
      "callers": ["2@16:3"],
      "uses": ["8:6", "16:3"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "foo",
      "definition": "10:6",
      "callees": ["0@14:3", "0@15:3", "1@16:3", "0@17:3"],
      "uses": ["10:6"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@x",
      "short_name": "x",
      "qualified_name": "Foo::x",
      "definition": "3:7",
      "declaring_type": 0,
      "uses": ["3:7", "12:5", "13:5", "14:12", "15:12", "16:13"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@y",
      "short_name": "y",
      "qualified_name": "Foo::y",
      "definition": "4:7",
      "declaring_type": 0,
      "uses": ["4:7", "17:12"]
    }, {
      "id": 2,
      "usr": "c:var_usage_class_member.cc@105@F@foo#@f",
      "short_name": "f",
      "qualified_name": "f",
      "definition": "11:7",
      "variable_type": 0,
      "uses": ["11:7", "12:3", "13:3", "14:10", "15:10", "16:11", "17:10"]
    }]
}
*/
