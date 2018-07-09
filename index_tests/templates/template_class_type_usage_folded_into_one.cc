enum A {};
enum B {};

template<typename T>
struct Foo {
  struct Inner {};
};

Foo<A>::Inner a;
Foo<B>::Inner b;

#if false
EnumDecl A
EnumDecl B
ClassTemplate Foo
  TemplateTypeParameter T
  StructDecl Inner
VarDecl a
  TemplateRef Foo
  TypeRef enum A
  TypeRef struct Foo<enum A>::Inner
  CallExpr Inner
VarDecl b
  TemplateRef Foo
  TypeRef enum B
  TypeRef struct Foo<enum B>::Inner
  CallExpr Inner
#endif

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": ["12:1-29:1"],
  "usr2func": [],
  "usr2type": [{
      "usr": 6697181287623958829,
      "detailed_name": "enum A {\n}",
      "qual_name_offset": 5,
      "short_name": "A",
      "kind": 10,
      "declarations": [],
      "spell": "1:6-1:7|0|1|2",
      "extent": "1:1-1:10|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["9:5-9:6|0|1|4"]
    }, {
      "usr": 7074603899792463171,
      "detailed_name": "Inner",
      "qual_name_offset": 0,
      "short_name": "Inner",
      "kind": 26,
      "declarations": [],
      "spell": "6:3-6:18|0|1|2",
      "extent": "6:3-6:18|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [16721564935990383768],
      "uses": []
    }, {
      "usr": 10528472276654770367,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "5:8-5:11|0|1|2",
      "extent": "5:1-7:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [13938528237873543349],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["9:1-9:4|0|1|4", "10:1-10:4|0|1|4"]
    }, {
      "usr": 13892793056005362145,
      "detailed_name": "enum B {\n}",
      "qual_name_offset": 5,
      "short_name": "B",
      "kind": 10,
      "declarations": [],
      "spell": "2:6-2:7|0|1|2",
      "extent": "2:1-2:10|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["10:5-10:6|0|1|4"]
    }, {
      "usr": 13938528237873543349,
      "detailed_name": "struct Foo::Inner {}",
      "qual_name_offset": 7,
      "short_name": "Inner",
      "kind": 23,
      "declarations": [],
      "spell": "6:10-6:15|10528472276654770367|2|514",
      "extent": "6:3-6:18|10528472276654770367|2|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["9:9-9:14|10528472276654770367|2|4", "10:9-10:14|10528472276654770367|2|4"]
    }, {
      "usr": 15961308565836244174,
      "detailed_name": "Inner",
      "qual_name_offset": 0,
      "short_name": "Inner",
      "kind": 26,
      "declarations": [],
      "spell": "6:3-6:18|0|1|2",
      "extent": "6:3-6:18|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [12028309045033782423],
      "uses": []
    }],
  "usr2var": [{
      "usr": 12028309045033782423,
      "detailed_name": "Foo<B>::Inner b",
      "qual_name_offset": 14,
      "short_name": "b",
      "declarations": [],
      "spell": "10:15-10:16|0|1|2",
      "extent": "10:1-10:16|0|1|0",
      "type": 15961308565836244174,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "Foo<A>::Inner a",
      "qual_name_offset": 14,
      "short_name": "a",
      "declarations": [],
      "spell": "9:15-9:16|0|1|2",
      "extent": "9:1-9:16|0|1|0",
      "type": 7074603899792463171,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
