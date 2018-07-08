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
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "2:6-2:7|0|1|2",
      "extent": "2:1-5:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [10601729374837386290, 18422884837902130475],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|0|1|2",
      "extent": "1:1-1:13|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [10601729374837386290, 18422884837902130475],
      "uses": ["3:16-3:19|0|1|4", "4:17-4:20|0|1|4"]
    }],
  "usr2var": [{
      "usr": 10601729374837386290,
      "detailed_name": "auto x",
      "qual_name_offset": 5,
      "short_name": "x",
      "hover": "auto x = new Foo()",
      "declarations": [],
      "spell": "3:8-3:9|880549676430489861|3|2",
      "extent": "3:3-3:21|880549676430489861|3|0",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 18422884837902130475,
      "detailed_name": "Foo *y",
      "qual_name_offset": 5,
      "short_name": "y",
      "hover": "Foo *y = new Foo()",
      "declarations": [],
      "spell": "4:9-4:10|880549676430489861|3|2",
      "extent": "4:3-4:22|880549676430489861|3|0",
      "type": 15041163540773201510,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
