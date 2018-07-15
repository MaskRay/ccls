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
      "kind": 6,
      "storage": 0,
      "declarations": ["3:8-3:16|15041163540773201510|2|1025"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 3986818119971932909,
      "detailed_name": "int Foo::operator()(int a, int b)",
      "qual_name_offset": 4,
      "short_name": "operator()",
      "kind": 6,
      "storage": 0,
      "declarations": ["4:7-4:15|15041163540773201510|2|1025"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 7874436189163837815,
      "detailed_name": "void Foo::operator()(int)",
      "qual_name_offset": 5,
      "short_name": "operator()",
      "kind": 6,
      "storage": 0,
      "declarations": [],
      "spell": "2:8-2:16|15041163540773201510|2|1026",
      "extent": "2:3-2:27|15041163540773201510|2|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 8288368475529136092,
      "detailed_name": "Foo &operator+=(const Foo &, const int &)",
      "qual_name_offset": 5,
      "short_name": "operator+=",
      "kind": 12,
      "storage": 0,
      "declarations": ["7:6-7:14|0|1|1"],
      "bases": [],
      "derived": [],
      "vars": [],
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
      "extent": "1:1-5:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [7874436189163837815, 3545323327609582678, 3986818119971932909],
      "vars": [],
      "instances": [],
      "uses": ["7:1-7:4|0|1|4", "7:25-7:28|0|1|4"]
    }],
  "usr2var": []
}
*/
