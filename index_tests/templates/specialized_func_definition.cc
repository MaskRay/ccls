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
      "detailed_name": "void Template<void>::Foo()",
      "qual_name_offset": 5,
      "short_name": "Foo",
      "spell": "10:22-10:25|9:1-10:30|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 5,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 11994188353303124840,
      "detailed_name": "void Template::Foo()",
      "qual_name_offset": 5,
      "short_name": "Foo",
      "spell": "7:19-7:22|6:1-7:24|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 5,
      "storage": 0,
      "declarations": ["3:8-3:11|3:3-3:13|1025|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 17107291254533526269,
      "detailed_name": "class Template {}",
      "qual_name_offset": 6,
      "short_name": "Template",
      "spell": "2:7-2:15|2:1-4:2|2|-1",
      "bases": [],
      "funcs": [11994188353303124840],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["7:6-7:14|4|-1", "10:6-10:14|4|-1"]
    }, {
      "usr": 17649312483543982122,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [6995843774014807426],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
