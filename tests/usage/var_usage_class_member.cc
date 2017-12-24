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
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-5:2",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0, 1],
      "instances": [2],
      "uses": ["1:7-1:10", "11:3-11:6"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@accept#I#",
      "short_name": "accept",
      "detailed_name": "void accept(int)",
      "declarations": [{
          "spelling": "7:6-7:12",
          "extent": "7:1-7:17",
          "content": "void accept(int)",
          "param_spellings": ["7:16-7:16"]
        }],
      "base": [],
      "derived": [],
      "locals": [],
      "callers": ["2@14:3-14:9", "2@15:3-15:9", "2@17:3-17:9"],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@F@accept#*I#",
      "short_name": "accept",
      "detailed_name": "void accept(int *)",
      "declarations": [{
          "spelling": "8:6-8:12",
          "extent": "8:1-8:18",
          "content": "void accept(int*)",
          "param_spellings": ["8:17-8:17"]
        }],
      "base": [],
      "derived": [],
      "locals": [],
      "callers": ["2@16:3-16:9"],
      "callees": []
    }, {
      "id": 2,
      "is_operator": false,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "declarations": [],
      "definition_spelling": "10:6-10:9",
      "definition_extent": "10:1-18:2",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["0@14:3-14:9", "0@15:3-15:9", "1@16:3-16:9", "0@17:3-17:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@S@Foo@FI@x",
      "short_name": "x",
      "detailed_name": "int Foo::x",
      "definition_spelling": "3:7-3:8",
      "definition_extent": "3:3-3:8",
      "declaring_type": 0,
      "cls": 4,
      "uses": ["3:7-3:8", "12:5-12:6", "13:5-13:6", "14:12-14:13", "15:12-15:13", "16:13-16:14"]
    }, {
      "id": 1,
      "usr": "c:@S@Foo@FI@y",
      "short_name": "y",
      "detailed_name": "int Foo::y",
      "definition_spelling": "4:7-4:8",
      "definition_extent": "4:3-4:8",
      "declaring_type": 0,
      "cls": 4,
      "uses": ["4:7-4:8", "17:12-17:13"]
    }, {
      "id": 2,
      "usr": "c:var_usage_class_member.cc@105@F@foo#@f",
      "short_name": "f",
      "detailed_name": "Foo f",
      "definition_spelling": "11:7-11:8",
      "definition_extent": "11:3-11:8",
      "variable_type": 0,
      "cls": 1,
      "uses": ["11:7-11:8", "12:3-12:4", "13:3-13:4", "14:10-14:11", "15:10-15:11", "16:11-16:12", "17:10-17:11"]
    }]
}
*/
