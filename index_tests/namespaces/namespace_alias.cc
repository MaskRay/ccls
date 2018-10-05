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
      "spell": "11:6-11:10|11:1-14:2|2|-1",
      "bases": [],
      "vars": [6030927277961448585, 7657277353101371136],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [15042442838933090518, 6030927277961448585, 7657277353101371136],
      "uses": []
    }, {
      "usr": 926793467007732869,
      "detailed_name": "namespace foo {}",
      "qual_name_offset": 10,
      "short_name": "foo",
      "bases": [],
      "funcs": [],
      "types": [17805385787823406700],
      "vars": [],
      "alias_of": 0,
      "kind": 3,
      "parent_kind": 0,
      "declarations": ["1:11-1:14|1:1-7:2|1|-1"],
      "derived": [17805385787823406700],
      "instances": [],
      "uses": ["9:17-9:20|4|-1", "12:11-12:14|4|-1"]
    }, {
      "usr": 11879713791858506216,
      "detailed_name": "namespace fbz = foo::bar::baz",
      "qual_name_offset": 10,
      "short_name": "fbz",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 14450849931009540802,
      "kind": 252,
      "parent_kind": 0,
      "declarations": ["9:11-9:14|9:1-9:30|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["13:11-13:14|4|-1"]
    }, {
      "usr": 14450849931009540802,
      "detailed_name": "namespace foo::bar::baz {}",
      "qual_name_offset": 10,
      "short_name": "baz",
      "bases": [17805385787823406700],
      "funcs": [],
      "types": [],
      "vars": [{
          "L": 15042442838933090518,
          "R": -1
        }],
      "alias_of": 0,
      "kind": 3,
      "parent_kind": 0,
      "declarations": ["3:20-3:23|3:10-5:11|1025|-1"],
      "derived": [],
      "instances": [],
      "uses": ["9:27-9:30|4|-1", "12:21-12:24|4|-1"]
    }, {
      "usr": 17805385787823406700,
      "detailed_name": "namespace foo::bar {}",
      "qual_name_offset": 10,
      "short_name": "bar",
      "bases": [926793467007732869],
      "funcs": [],
      "types": [14450849931009540802],
      "vars": [],
      "alias_of": 0,
      "kind": 3,
      "parent_kind": 0,
      "declarations": ["2:15-2:18|2:5-6:6|1025|-1"],
      "derived": [14450849931009540802],
      "instances": [],
      "uses": ["9:22-9:25|4|-1", "12:16-12:19|4|-1"]
    }],
  "usr2var": [{
      "usr": 6030927277961448585,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "hover": "int a = foo::bar::baz::qux",
      "spell": "12:7-12:8|12:3-12:29|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 7657277353101371136,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "hover": "int b = fbz::qux",
      "spell": "13:7-13:8|13:3-13:19|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 15042442838933090518,
      "detailed_name": "int foo::bar::baz::qux",
      "qual_name_offset": 4,
      "short_name": "qux",
      "hover": "int foo::bar::baz::qux = 42",
      "spell": "4:18-4:21|4:14-4:26|1026|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 3,
      "storage": 0,
      "declarations": [],
      "uses": ["12:26-12:29|12|-1", "13:16-13:19|12|-1"]
    }]
}
*/
