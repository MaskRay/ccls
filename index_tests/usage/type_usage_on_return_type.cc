struct Type;

Type* foo();
Type* foo();
Type* foo() { return nullptr; }

class Foo {
  Type* Get(int);
  void Empty();
};

Type* Foo::Get(int) { return nullptr; }
void Foo::Empty() {}

extern const Type& external();

static Type* bar();
static Type* bar() { return nullptr; }

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 4240751906910175539,
      "detailed_name": "void Foo::Empty()",
      "qual_name_offset": 5,
      "short_name": "Empty",
      "kind": 6,
      "storage": 0,
      "declarations": ["9:8-9:13|9:3-9:15|15041163540773201510|2|1025|-1"],
      "spell": "13:11-13:16|15041163540773201510|2|1026|-1",
      "extent": "13:1-13:21|15041163540773201510|2|0|-1",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 4259594751088586730,
      "detailed_name": "Type *foo()",
      "qual_name_offset": 6,
      "short_name": "foo",
      "kind": 12,
      "storage": 0,
      "declarations": ["3:7-3:10|3:1-3:12|0|1|1|-1", "4:7-4:10|4:1-4:12|0|1|1|-1"],
      "spell": "5:7-5:10|0|1|2|-1",
      "extent": "5:1-5:32|0|1|0|-1",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 7746867874366499515,
      "detailed_name": "extern const Type &external()",
      "qual_name_offset": 19,
      "short_name": "external",
      "kind": 12,
      "storage": 0,
      "declarations": ["15:20-15:28|15:1-15:30|0|1|1|-1"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 13402221340333431092,
      "detailed_name": "Type *Foo::Get(int)",
      "qual_name_offset": 6,
      "short_name": "Get",
      "kind": 6,
      "storage": 0,
      "declarations": ["8:9-8:12|8:3-8:17|15041163540773201510|2|1025|-1"],
      "spell": "12:12-12:15|15041163540773201510|2|1026|-1",
      "extent": "12:1-12:40|15041163540773201510|2|0|-1",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 18408440185620243373,
      "detailed_name": "static Type *bar()",
      "qual_name_offset": 13,
      "short_name": "bar",
      "kind": 12,
      "storage": 0,
      "declarations": ["17:14-17:17|17:1-17:19|0|1|1|-1"],
      "spell": "18:14-18:17|0|1|2|-1",
      "extent": "18:1-18:39|0|1|0|-1",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 13487927231218873822,
      "detailed_name": "struct Type",
      "qual_name_offset": 7,
      "short_name": "Type",
      "kind": 23,
      "declarations": ["1:8-1:12|1:1-1:12|0|1|1|-1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["3:1-3:5|0|1|4|-1", "4:1-4:5|0|1|4|-1", "5:1-5:5|0|1|4|-1", "8:3-8:7|15041163540773201510|2|4|-1", "12:1-12:5|0|1|4|-1", "15:14-15:18|0|1|4|-1", "17:8-17:12|0|1|4|-1", "18:8-18:12|0|1|4|-1"]
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "7:7-7:10|0|1|2|-1",
      "extent": "7:1-10:2|0|1|0|-1",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [13402221340333431092, 4240751906910175539],
      "vars": [],
      "instances": [],
      "uses": ["12:7-12:10|0|1|4|-1", "13:6-13:9|0|1|4|-1"]
    }],
  "usr2var": []
}
*/
