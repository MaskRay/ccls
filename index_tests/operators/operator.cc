class Foo {
  void operator()(int) { }
  void operator()(bool);
  int operator()(int a, int b);
};

Foo &operator += (const Foo&, const int&);

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 3545323327609582678,
      "detailed_name": "void Foo::operator()(bool)",
      "qual_name_offset": 5,
      "short_name": "operator()",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["3:8-3:16|3:3-3:24|1025|-1"],
      "derived": [],
      "uses": []
    }, {
      "usr": 3986818119971932909,
      "detailed_name": "int Foo::operator()(int a, int b)",
      "qual_name_offset": 4,
      "short_name": "operator()",
      "bases": [],
      "vars": [18194802024223591994, 3165816734756776484],
      "callees": [],
      "kind": 6,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["4:7-4:15|4:3-4:31|1025|-1"],
      "derived": [],
      "uses": []
    }, {
      "usr": 7874436189163837815,
      "detailed_name": "void Foo::operator()(int)",
      "qual_name_offset": 5,
      "short_name": "operator()",
      "spell": "2:8-2:18|2:3-2:27|1026|-1",
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
      "usr": 8288368475529136092,
      "detailed_name": "Foo &operator+=(const Foo &, const int &)",
      "qual_name_offset": 5,
      "short_name": "operator+=",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["7:6-7:14|7:1-7:42|1|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 452,
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
      "instances": [18194802024223591994, 3165816734756776484],
      "uses": []
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "spell": "1:7-1:10|1:1-5:2|2|-1",
      "bases": [],
      "funcs": [7874436189163837815, 3545323327609582678, 3986818119971932909],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 1,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["7:1-7:4|4|-1", "7:25-7:28|4|-1"]
    }],
  "usr2var": [{
      "usr": 3165816734756776484,
      "detailed_name": "int b",
      "qual_name_offset": 4,
      "short_name": "b",
      "spell": "4:29-4:30|4:25-4:30|1026|-1",
      "type": 452,
      "kind": 253,
      "parent_kind": 6,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 18194802024223591994,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "spell": "4:22-4:23|4:18-4:23|1026|-1",
      "type": 452,
      "kind": 253,
      "parent_kind": 6,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
