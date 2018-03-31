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
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 15041163540773201510,
      "detailed_name": "Foo",
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:10|-1|1|2",
      "extent": "1:1-5:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [0, 1, 2],
      "vars": [],
      "instances": [],
      "uses": ["7:1-7:4|-1|1|4", "7:25-7:28|-1|1|4"]
    }],
  "funcs": [{
      "id": 0,
      "usr": 7874436189163837815,
      "detailed_name": "void Foo::operator()(int)",
      "short_name": "operator()",
      "kind": 6,
      "storage": 1,
      "declarations": [],
      "spell": "2:8-2:18|0|2|2",
      "extent": "2:3-2:27|0|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 1,
      "usr": 3545323327609582678,
      "detailed_name": "void Foo::operator()(bool)",
      "short_name": "operator()",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "3:8-3:18|0|2|1",
          "param_spellings": ["3:23-3:23"]
        }],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 2,
      "usr": 3986818119971932909,
      "detailed_name": "int Foo::operator()(int a, int b)",
      "short_name": "operator()",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "4:7-4:17|0|2|1",
          "param_spellings": ["4:22-4:23", "4:29-4:30"]
        }],
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 3,
      "usr": 8288368475529136092,
      "detailed_name": "Foo &operator+=(const Foo &, const int &)",
      "short_name": "operator+=",
      "kind": 12,
      "storage": 1,
      "declarations": [{
          "spell": "7:6-7:17|-1|1|1",
          "param_spellings": ["7:29-7:29", "7:41-7:41"]
        }],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
*/
