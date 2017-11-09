int foo(int, int);
int foo(int aa,
        int bb);
int foo(int aaa, int bbb);
int foo(int a, int b) { return 0; }

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#I#I#",
      "short_name": "foo",
      "detailed_name": "int foo(int, int)",
      "declarations": [{
          "spelling": "1:5-1:8",
          "extent": "1:1-1:18",
          "content": "int foo(int, int)",
          "param_spellings": ["1:12-1:12", "1:17-1:17"]
        }, {
          "spelling": "2:5-2:8",
          "extent": "2:1-3:16",
          "content": "int foo(int aa,\n         int bb)",
          "param_spellings": ["2:13-2:15", "3:13-3:15"]
        }, {
          "spelling": "4:5-4:8",
          "extent": "4:1-4:26",
          "content": "int foo(int aaa, int bbb)",
          "param_spellings": ["4:13-4:16", "4:22-4:25"]
        }],
      "definition_spelling": "5:5-5:8",
      "definition_extent": "5:1-5:36"
    }],
  "vars": [{
      "id": 0,
      "usr": "c:func_associated_function_params.cc@91@F@foo#I#I#@a",
      "short_name": "a",
      "detailed_name": "int a",
      "definition_spelling": "5:13-5:14",
      "definition_extent": "5:9-5:14",
      "is_local": true,
      "is_macro": false,
      "uses": ["5:13-5:14"]
    }, {
      "id": 1,
      "usr": "c:func_associated_function_params.cc@98@F@foo#I#I#@b",
      "short_name": "b",
      "detailed_name": "int b",
      "definition_spelling": "5:20-5:21",
      "definition_extent": "5:16-5:21",
      "is_local": true,
      "is_macro": false,
      "uses": ["5:20-5:21"]
    }]
}
*/
