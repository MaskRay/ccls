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
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": "c:lambda.cc@47@F@foo#@Sa",
      "short_name": "",
      "detailed_name": "",
      "hover": "",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [1],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "hover": "void foo()",
      "declarations": [],
      "definition_spelling": "1:6-1:9",
      "definition_extent": "1:1-12:2",
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": ["1@9:14-9:15", "1@10:14-10:15", "1@11:14-11:15"]
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:lambda.cc@57@F@foo#@Sa@F@operator()#I#1",
      "short_name": "",
      "detailed_name": "",
      "hover": "",
      "declarations": [],
      "derived": [],
      "locals": [],
      "callers": ["0@9:14-9:15", "0@10:14-10:15", "0@11:14-11:15"],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": "c:lambda.cc@16@F@foo#@x",
      "short_name": "x",
      "detailed_name": "int x",
      "hover": "int",
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
      "hover": "(lambda at C:/Users/jacob/Desktop/cquery/tests/lambdas/lambda.cc:4:22)",
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
      "hover": "int",
      "definition_spelling": "4:31-4:32",
      "definition_extent": "4:27-4:32",
      "is_local": false,
      "is_macro": false,
      "uses": ["4:31-4:32", "6:7-6:8"]
    }]
}
*/
