void foo() {
  int x;

  auto dosomething = [&x](int y) {
    ++x;
    ++y;
  };

  dosomething(1);
  dosomething(1);
  dosomething(1);
}

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:lambda.cc@47@F@foo#@Sa",
      "short_name": "",
      "detailed_name": "",
      "instances": [1]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-12:2",
      "callees": ["1@9:14-9:15", "1@10:14-10:15", "1@11:14-11:15"]
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:lambda.cc@57@F@foo#@Sa@F@operator()#I#1",
      "short_name": "",
      "detailed_name": "",
      "callers": ["0@9:14-9:15", "0@10:14-10:15", "0@11:14-11:15"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:lambda.cc@16@F@foo#@x",
      "short_name": "x",
      "detailed_name": "int x",
      "definition_spelling": "2:7-2:8",
      "definition_extent": "2:3-2:8",
      "is_local": true,
      "is_macro": false,
      "uses": ["2:7-2:8", "5:7-5:8", "4:24-4:25"]
    }, {
      "id": 1,
      "usr": "c:lambda.cc@28@F@foo#@dosomething",
      "short_name": "dosomething",
      "detailed_name": "(lambda at C:/Users/jacob/Desktop/cquery/tests/lambdas/lambda.cc:4:22) dosomething",
      "definition_spelling": "4:8-4:19",
      "definition_extent": "4:3-7:4",
      "variable_type": 0,
      "is_local": true,
      "is_macro": false,
      "uses": ["4:8-4:19", "9:3-9:14", "10:3-10:14", "11:3-11:14"]
    }, {
      "id": 2,
      "usr": "c:lambda.cc@52@F@foo#@Sa@F@operator()#I#1@y",
      "short_name": "y",
      "detailed_name": "int y",
      "definition_spelling": "4:31-4:32",
      "definition_extent": "4:31-4:32",
      "is_local": false,
      "is_macro": false,
      "uses": ["4:31-4:32", "6:7-6:8"]
    }]
}
*/
