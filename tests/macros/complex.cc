#define FOO(aaa, bbb) \
  int a();\
  int a() { return aaa + bbb; }


int make1() {
  return 3;
}
const int make2 = 5;


FOO(make1(), make2);

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@make1#",
      "short_name": "make1",
      "detailed_name": "int make1()",
      "definition_spelling": "6:5-6:10",
      "definition_extent": "6:1-8:2",
      "callers": ["1@12:5-12:10"]
    }, {
      "id": 1,
      "usr": "c:@F@a#",
      "short_name": "a",
      "detailed_name": "int a()",
      "declarations": [{
          "spelling": "12:1-12:20",
          "extent": "12:1-12:20",
          "content": "int a();\n   int a() { return aaa + bbb; }\n\n\n int make1() {\n   return 3;\n }\n const int make2 = 5;\n\n\n FOO(make1(), make2)"
        }],
      "definition_spelling": "12:1-12:20",
      "definition_extent": "12:1-12:20",
      "callees": ["0@12:5-12:10"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:complex.cc@make2",
      "short_name": "make2",
      "detailed_name": "const int make2",
      "definition_spelling": "9:11-9:16",
      "definition_extent": "9:1-9:20",
      "is_local": false,
      "uses": ["9:11-9:16", "12:14-12:19"]
    }, {
      "id": 1,
      "usr": "c:complex.cc@8@macro@FOO",
      "short_name": "FOO",
      "detailed_name": "FOO",
      "definition_spelling": "1:9-1:12",
      "definition_extent": "1:9-3:32",
      "is_local": false,
      "uses": ["1:9-1:12", "12:1-12:4"]
    }]
}
*/