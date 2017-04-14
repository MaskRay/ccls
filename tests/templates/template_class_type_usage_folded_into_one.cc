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
  "types": [{
      "id": 0,
      "usr": "c:@E@A",
      "short_name": "A",
      "qualified_name": "A",
      "definition_spelling": "1:6-1:7",
      "definition_extent": "1:1-1:10",
      "uses": ["*1:6-1:7", "*9:5-9:6"]
    }, {
      "id": 1,
      "usr": "c:@E@B",
      "short_name": "B",
      "qualified_name": "B",
      "definition_spelling": "2:6-2:7",
      "definition_extent": "2:1-2:10",
      "uses": ["*2:6-2:7", "*10:5-10:6"]
    }, {
      "id": 2,
      "usr": "c:@ST>1#T@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition_spelling": "5:8-5:11",
      "definition_extent": "5:1-7:2",
      "uses": ["*5:8-5:11", "*9:1-9:4", "*10:1-10:4"]
    }, {
      "id": 3,
      "usr": "c:@ST>1#T@Foo@S@Inner",
      "short_name": "Inner",
      "qualified_name": "Foo::Inner",
      "definition_spelling": "6:10-6:15",
      "definition_extent": "6:3-6:18",
      "instantiations": [0, 1],
      "uses": ["*6:10-6:15", "*9:9-9:14", "*10:9-10:14"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "hover": "Foo<A>::Inner",
      "definition_spelling": "9:15-9:16",
      "definition_extent": "9:1-9:16",
      "variable_type": 3,
      "uses": ["9:15-9:16"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "hover": "Foo<B>::Inner",
      "definition_spelling": "10:15-10:16",
      "definition_extent": "10:1-10:16",
      "variable_type": 3,
      "uses": ["10:15-10:16"]
    }]
}
*/
