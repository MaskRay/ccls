template<class T>
class Template {
  void Foo();
};

template<class T>
void Template<T>::Foo() {}

template<>
void Template<void>::Foo() {}


/*
// TODO: usage information on Template is bad.
// TODO: Foo() should have multiple definitions.

EXTRA_FLAGS:
-fms-compatibility
-fdelayed-template-parsing

OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 6995843774014807426,
      "detailed_name": "template <> void Template<void>::Foo()",
      "qual_name_offset": 31,
      "short_name": "Foo",
      "kind": 6,
      "storage": 0,
      "declarations": [],
      "spell": "10:22-10:25|17649312483543982122|2|1026",
      "extent": "9:1-10:30|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 11994188353303124840,
      "detailed_name": "void Template::Foo()",
      "qual_name_offset": 5,
      "short_name": "Foo",
      "kind": 6,
      "storage": 0,
      "declarations": ["3:8-3:11|17107291254533526269|2|1025"],
      "spell": "7:19-7:22|17107291254533526269|2|1026",
      "extent": "6:1-7:24|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 17107291254533526269,
      "detailed_name": "class Template {}",
      "qual_name_offset": 6,
      "short_name": "Template",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:15|0|1|2",
      "extent": "2:1-4:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [11994188353303124840],
      "vars": [],
      "instances": [],
      "uses": ["7:6-7:14|0|1|4", "10:6-10:14|0|1|4"]
    }, {
      "usr": 17649312483543982122,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [6995843774014807426],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
