namespace foo {
    namespace bar {
         namespace baz {
             int qux = 42;
         }
    }
}

namespace fbz = foo::bar::baz;

void func() {
  int a = foo::bar::baz::qux;
  int b = fbz::qux;
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 926793467007732869,
      "detailed_name": "foo",
      "short_name": "foo",
      "kind": 3,
      "declarations": [],
      "spell": "1:11-1:14|-1|1|2",
      "extent": "1:1-7:2|-1|1|0",
      "bases": [1],
      "derived": [2],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["1:11-1:14|-1|1|4", "9:17-9:20|-1|1|4", "12:11-12:14|0|3|4"]
    }, {
      "id": 1,
      "usr": 13838176792705659279,
      "detailed_name": "<fundamental>",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [0],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "id": 2,
      "usr": 17805385787823406700,
      "detailed_name": "foo::bar",
      "short_name": "bar",
      "kind": 3,
      "declarations": [],
      "spell": "2:15-2:18|0|2|2",
      "extent": "2:5-6:6|0|2|0",
      "bases": [0],
      "derived": [3],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:15-2:18|0|2|4", "9:22-9:25|-1|1|4", "12:16-12:19|0|3|4"]
    }, {
      "id": 3,
      "usr": 14450849931009540802,
      "detailed_name": "foo::bar::baz",
      "short_name": "baz",
      "kind": 3,
      "declarations": [],
      "spell": "3:20-3:23|2|2|2",
      "extent": "3:10-5:11|2|2|0",
      "bases": [2],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [0],
      "instances": [],
      "uses": ["3:20-3:23|2|2|4", "9:27-9:30|-1|1|4", "12:21-12:24|0|3|4"]
    }, {
      "id": 4,
      "usr": 17,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1, 2],
      "uses": []
    }, {
      "id": 5,
      "usr": 11879713791858506216,
      "detailed_name": "fbz",
      "short_name": "fbz",
      "kind": 0,
      "declarations": [],
      "spell": "9:11-9:14|-1|1|2",
      "extent": "9:1-9:30|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:11-13:14|0|3|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 10818727483146447186,
      "detailed_name": "void func()",
      "short_name": "func",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "11:6-11:10|-1|1|2",
      "extent": "11:1-14:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [1, 2],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 15042442838933090518,
      "detailed_name": "int foo::bar::baz::qux",
      "short_name": "qux",
      "hover": "int foo::bar::baz::qux = 42",
      "declarations": [],
      "spell": "4:18-4:21|3|2|2",
      "extent": "4:14-4:26|3|2|0",
      "type": 4,
      "uses": ["12:26-12:29|-1|1|4", "13:16-13:19|-1|1|4"],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 107714981785063096,
      "detailed_name": "int a",
      "short_name": "a",
      "hover": "int a = foo::bar::baz::qux",
      "declarations": [],
      "spell": "12:7-12:8|0|3|2",
      "extent": "12:3-12:29|0|3|0",
      "type": 4,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 2,
      "usr": 1200087780658383286,
      "detailed_name": "int b",
      "short_name": "b",
      "hover": "int b = fbz::qux",
      "declarations": [],
      "spell": "13:7-13:8|0|3|2",
      "extent": "13:3-13:19|0|3|0",
      "type": 4,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
