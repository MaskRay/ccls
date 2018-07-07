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
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 10818727483146447186,
      "detailed_name": "void func()",
      "qual_name_offset": 5,
      "short_name": "func",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "11:6-11:10|0|1|2",
      "extent": "11:1-14:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 14450849931009540802,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "storage": 0,
      "declarations": [],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [15042442838933090518],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [15042442838933090518, 6030927277961448585, 7657277353101371136],
      "uses": []
    }, {
      "usr": 926793467007732869,
      "detailed_name": "namespace foo {\n}",
      "qual_name_offset": 10,
      "short_name": "foo",
      "kind": 3,
      "declarations": ["1:11-1:14|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [17805385787823406700],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["9:17-9:20|0|1|4", "12:11-12:14|0|1|4"]
    }, {
      "usr": 11879713791858506216,
      "detailed_name": "namespace fbz = foo::bar::baz",
      "qual_name_offset": 10,
      "short_name": "fbz",
      "kind": 252,
      "declarations": ["9:11-9:14|0|1|1"],
      "alias_of": 14450849931009540802,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:11-13:14|0|1|4"]
    }, {
      "usr": 14450849931009540802,
      "detailed_name": "namespace foo::bar::baz {\n}",
      "qual_name_offset": 10,
      "short_name": "baz",
      "kind": 3,
      "declarations": ["3:20-3:23|17805385787823406700|2|513"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["9:27-9:30|17805385787823406700|2|4", "12:21-12:24|17805385787823406700|2|4"]
    }, {
      "usr": 17805385787823406700,
      "detailed_name": "namespace foo::bar {\n}",
      "qual_name_offset": 10,
      "short_name": "bar",
      "kind": 3,
      "declarations": ["2:15-2:18|926793467007732869|2|513"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [14450849931009540802],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["9:22-9:25|926793467007732869|2|4", "12:16-12:19|926793467007732869|2|4"]
    }],
  "usr2var": [{
      "usr": 6030927277961448585,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "hover": "int a = foo::bar::baz::qux",
      "declarations": [],
      "spell": "12:7-12:8|10818727483146447186|3|2",
      "extent": "12:3-12:29|10818727483146447186|3|0",
      "type": 53,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 7657277353101371136,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int b = fbz::qux",
      "declarations": [],
      "spell": "13:7-13:8|10818727483146447186|3|2",
      "extent": "13:3-13:19|10818727483146447186|3|0",
      "type": 53,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 15042442838933090518,
      "detailed_name": "int foo::bar::baz::qux",
      "qual_name_offset": 4,
      "short_name": "qux",
      "hover": "int foo::bar::baz::qux = 42",
      "declarations": [],
      "spell": "4:18-4:21|14450849931009540802|2|514",
      "extent": "4:14-4:26|14450849931009540802|2|0",
      "type": 53,
      "uses": ["12:26-12:29|14450849931009540802|2|12", "13:16-13:19|14450849931009540802|2|12"],
      "kind": 13,
      "storage": 0
    }]
}
*/
