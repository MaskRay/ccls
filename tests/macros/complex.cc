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
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@make1#",
      "short_name": "make1",
      "detailed_name": "int make1()",
      "hover": "int make1()",
      "declarations": [],
      "definition_spelling": "6:5-6:10",
      "definition_extent": "6:1-8:2",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": ["1@12:5-12:10"],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@F@a#",
      "short_name": "a",
      "detailed_name": "int a()",
      "hover": "int a()",
      "declarations": [{
          "spelling": "12:1-12:20",
          "extent": "12:1-12:20",
          "content": "int a();\n   int a() { return aaa + bbb; }\n\n\n int make1() {\n   return 3;\n }\n const int make2 = 5;\n\n\n FOO(make1(), make2)",
          "param_spellings": []
        }],
      "definition_spelling": "12:1-12:20",
      "definition_extent": "12:1-12:20",
      "base": [],
      "derived": [],
      "locals": [],
      "callers": [],
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
      "is_macro": false,
      "is_global": false,
      "is_member": false,
      "uses": ["9:11-9:16", "12:14-12:19"]
    }, {
      "id": 1,
      "usr": "c:complex.cc@8@macro@FOO",
      "short_name": "FOO",
      "detailed_name": "FOO",
      "hover": "#define FOO(aaa, bbb)\n   int a();\n   int a() { return aaa + bbb; }",
      "definition_spelling": "1:9-1:12",
      "definition_extent": "1:9-3:32",
      "is_local": false,
      "is_macro": true,
      "is_global": false,
      "is_member": false,
      "uses": ["1:9-1:12", "12:1-12:4"]
    }]
}
*/