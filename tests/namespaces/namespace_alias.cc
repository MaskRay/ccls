namespace foo {
    namespace bar {
         namespace baz {
             int qux = 42;
         }
    }
}
 
namespace fbz = foo::bar::baz;

void foo() {
  int a = foo::bar::baz::qux;
  int b = fbz::qux;
}

/*
OUTPUT:
{
  "funcs": [{
      "id": 0,
      "usr": "c:@F@foo#",
      "short_name": "foo",
      "detailed_name": "void foo()",
      "definition_spelling": "11:6-11:9",
      "definition_extent": "11:1-14:2"
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@N@foo@N@bar@N@baz@qux",
      "short_name": "qux",
      "detailed_name": "int foo::bar::baz::qux",
      "definition_spelling": "4:18-4:21",
      "definition_extent": "4:14-4:26",
      "is_local": false,
      "is_macro": false,
      "uses": ["4:18-4:21", "12:26-12:29", "13:16-13:19"]
    }, {
      "id": 1,
      "usr": "c:namespace_alias.cc@167@F@foo#@a",
      "short_name": "a",
      "detailed_name": "int a",
      "definition_spelling": "12:7-12:8",
      "definition_extent": "12:3-12:29",
      "is_local": true,
      "is_macro": false,
      "uses": ["12:7-12:8"]
    }, {
      "id": 2,
      "usr": "c:namespace_alias.cc@198@F@foo#@b",
      "short_name": "b",
      "detailed_name": "int b",
      "definition_spelling": "13:7-13:8",
      "definition_extent": "13:3-13:19",
      "is_local": true,
      "is_macro": false,
      "uses": ["13:7-13:8"]
    }]
}
*/
