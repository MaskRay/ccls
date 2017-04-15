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
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-5:2",
      "vars": [0, 1],
      "instantiations": [2],
      "uses": ["*1:7-1:10", "*11:3-11:6"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@accept#I#",
      "short_name": "accept",
      "qualified_name": "void accept(int)",
      "declarations": ["7:6-7:12"],
      "callers": ["2@14:3-14:9", "2@15:3-15:9", "2@17:3-17:9"]
    }, {
      "id": 1,
      "usr": "c:@F@accept#*I#",
      "short_name": "accept",
      "qualified_name": "void accept(int *)",
      "declarations": ["8:6-8:12"],
      "callers": ["2@16:3-16:9"]
    }, {
      "id": 2,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "qualified_name": "void foo()",
      "definition_spelling": "10:6-10:9",
      "definition_extent": "10:1-18:2",
      "callees": ["0@14:3-14:9", "0@15:3-15:9", "1@16:3-16:9", "0@17:3-17:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@x",
      "short_name": "x",
      "qualified_name": "int Foo::x",
      "definition_spelling": "3:7-3:8",
      "definition_extent": "3:3-3:8",
      "declaring_type": 0,
      "uses": ["3:7-3:8", "12:5-12:6", "13:5-13:6", "14:12-14:13", "15:12-15:13", "16:13-16:14"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@y",
      "short_name": "y",
      "qualified_name": "int Foo::y",
      "definition_spelling": "4:7-4:8",
      "definition_extent": "4:3-4:8",
      "declaring_type": 0,
      "uses": ["4:7-4:8", "17:12-17:13"]
    }, {
      "id": 2,
      "usr": "c:var_usage_class_member.cc@105@F@foo#@f",
      "short_name": "f",
      "qualified_name": "Foo f",
      "definition_spelling": "11:7-11:8",
      "definition_extent": "11:3-11:8",
      "variable_type": 0,
      "uses": ["11:7-11:8", "12:3-12:4", "13:3-13:4", "14:10-14:11", "15:10-15:11", "16:11-16:12", "17:10-17:11"]
    }]
}
*/
