class Foo {};
void f() {
  auto x = new Foo();
  auto* y = new Foo();
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 880549676430489861,
      "detailed_name": "void f()",
      "qual_name_offset": 5,
      "short_name": "f",
      "spell": "2:6-2:7|2:1-5:2|2|-1",
      "bases": [],
      "vars": [10601729374837386290, 18422884837902130475],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "spell": "1:7-1:10|1:1-1:13|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [10601729374837386290, 18422884837902130475],
      "uses": ["3:16-3:19|4|-1", "4:17-4:20|4|-1"]
    }],
  "usr2var": [{
      "usr": 10601729374837386290,
      "detailed_name": "Foo *x",
      "qual_name_offset": 5,
      "short_name": "x",
      "hover": "Foo *x = new Foo()",
      "spell": "3:8-3:9|3:3-3:21|2|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 18422884837902130475,
      "detailed_name": "Foo *y",
      "qual_name_offset": 5,
      "short_name": "y",
      "hover": "Foo *y = new Foo()",
      "spell": "4:9-4:10|4:3-4:22|2|-1",
      "type": 15041163540773201510,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
