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
      "definition": "1:1:6",
      "uses": ["*1:1:6", "*1:9:5"]
    }, {
      "id": 1,
      "usr": "c:@E@B",
      "short_name": "B",
      "qualified_name": "B",
      "definition": "1:2:6",
      "uses": ["*1:2:6", "*1:10:5"]
    }, {
      "id": 2,
      "usr": "c:@ST>1#T@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "1:5:8",
      "uses": ["*1:5:8", "*1:9:1", "*1:10:1"]
    }, {
      "id": 3,
      "usr": "c:@ST>1#T@Foo@S@Inner",
      "short_name": "Inner",
      "qualified_name": "Foo::Inner",
      "definition": "1:6:10",
      "uses": ["*1:6:10", "*1:9:9", "*1:10:9"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:9:15",
      "variable_type": 3,
      "uses": ["1:9:15"]
    }, {
      "id": 1,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition": "1:10:15",
      "variable_type": 3,
      "uses": ["1:10:15"]
    }]
}
*/
