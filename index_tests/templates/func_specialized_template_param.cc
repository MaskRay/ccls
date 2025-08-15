template<class T>
class Template {};

struct Foo {
  void Bar(Template<double>&);
};

void Foo::Bar(Template<double>&) {}

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 8412238651648388423,
      "detailed_name": "void Foo::Bar(Template<double> &)",
      "qual_name_offset": 5,
      "short_name": "Bar",
      "spell": "8:11-8:14|8:1-8:36|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 23,
      "storage": 0,
      "declarations": ["5:8-5:11|5:3-5:30|1025|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 3122724794825267268,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 26,
      "parent_kind": 0,
      "declarations": ["1:16-1:17|1:10-1:17|1025|-1"],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "4:8-4:11|4:1-6:2|2|-1",
      "bases": [],
      "funcs": [8412238651648388423],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 1,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["8:6-8:9|4|-1"]
    }, {
      "usr": 17107291254533526269,
      "detailed_name": "class Template {}",
      "qual_name_offset": 6,
      "short_name": "Template",
      "spell": "2:7-2:15|2:1-2:18|2|-1",
      "bases": [],
      "funcs": [],
      "types": [3122724794825267268],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 1,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["5:12-5:20|4|-1", "8:15-8:23|4|-1"]
    }],
  "usr2var": []
}
*/
