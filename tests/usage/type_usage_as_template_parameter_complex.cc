template<typename T, typename B>
class unique_ptr;

struct S1;
struct S2;

#if false
VarDecl f
  TemplateRef unique_ptr
  TemplateRef unique_ptr
  TypeRef struct S1
  TypeRef struct S2
  TypeRef struct S2
#endif
extern unique_ptr<unique_ptr<S1, S2>, S2> f;

#if false
FunctionDecl as_return_type
  TemplateRef unique_ptr
  TemplateRef unique_ptr
  TypeRef struct S1
  TypeRef struct S2
  TypeRef struct S2
  ParmDecl
    TemplateRef unique_ptr
    TypeRef struct S1
    TypeRef struct S2
  CompoundStmt
    ReturnStmt
      UnexposedExpr
        CXXNullPtrLiteralExpr
#endif
unique_ptr<unique_ptr<S1, S2>, S2>* as_return_type(unique_ptr<S1, S2>*) { return nullptr; }

#if false
FunctionDecl no_return_type
  ParmDecl
  CompoundStmt
#endif
void no_return_type(int) {}

#if false
FunctionDecl empty
  CompoundStmt
    DeclStmt
      VarDecl local
        TemplateRef unique_ptr
        TemplateRef unique_ptr
        TypeRef struct S1
        TypeRef struct S2
        TypeRef struct S2
#endif
void empty() {
  unique_ptr<unique_ptr<S1, S2>, S2>* local;
}

#if false
ClassDecl Foo
  CXXMethod foo
    TemplateRef unique_ptr
    TypeRef struct S1
    TypeRef struct S2
#endif
class Foo {
  unique_ptr<S1, S2>* foo();
};

#if false
CXXMethod foo
  TemplateRef unique_ptr
  TypeRef struct S1
  TypeRef struct S2
  TypeRef class Foo
  CompoundStmt
    ReturnStmt
      UnexposedExpr
        CXXNullPtrLiteralExpr
#endif
unique_ptr<S1, S2>* Foo::foo() { return nullptr; }

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@ST>2#T#T@unique_ptr",
      "all_uses": ["*1:2:7", "*1:15:8", "*1:15:19", "*1:33:1", "*1:33:12", "*1:33:52", "*1:54:3", "*1:54:14", "*1:65:3", "*1:79:1"],
      "interesting_uses": ["*1:15:8", "*1:15:19", "*1:33:1", "*1:33:12", "*1:33:52", "*1:54:3", "*1:54:14", "*1:65:3", "*1:79:1"]
    }, {
      "id": 1,
      "usr": "c:@S@S1",
      "all_uses": ["*1:4:8", "*1:15:30", "*1:33:23", "*1:33:63", "*1:54:25", "*1:65:14", "*1:79:12"],
      "interesting_uses": ["*1:15:30", "*1:33:23", "*1:33:63", "*1:54:25", "*1:65:14", "*1:79:12"]
    }, {
      "id": 2,
      "usr": "c:@S@S2",
      "all_uses": ["*1:5:8", "*1:15:34", "*1:15:39", "*1:33:27", "*1:33:32", "*1:33:67", "*1:54:29", "*1:54:34", "*1:65:18", "*1:79:16"],
      "interesting_uses": ["*1:15:34", "*1:15:39", "*1:33:27", "*1:33:32", "*1:33:67", "*1:54:29", "*1:54:34", "*1:65:18", "*1:79:16"]
    }, {
      "id": 3,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition": "*1:64:7",
      "funcs": [3],
      "all_uses": ["*1:64:7", "*1:79:21"]
    }],
  "functions": [{
      "id": 0,
      "usr": "c:@F@as_return_type#*$@S@unique_ptr>#$@S@S1#$@S@S2#",
      "short_name": "as_return_type",
      "qualified_name": "as_return_type",
      "definition": "*1:33:37",
      "all_uses": ["*1:33:37"]
    }, {
      "id": 1,
      "usr": "c:@F@no_return_type#I#",
      "short_name": "no_return_type",
      "qualified_name": "no_return_type",
      "definition": "*1:40:6",
      "all_uses": ["*1:40:6"]
    }, {
      "id": 2,
      "usr": "c:@F@empty#",
      "short_name": "empty",
      "qualified_name": "empty",
      "definition": "*1:53:6",
      "all_uses": ["*1:53:6"]
    }, {
      "id": 3,
      "usr": "c:@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "declaration": "*1:65:23",
      "definition": "*1:79:26",
      "declaring_type": 3,
      "all_uses": ["*1:65:23", "*1:79:26"]
    }],
  "variables": [{
      "id": 0,
      "usr": "c:@f",
      "short_name": "f",
      "qualified_name": "f",
      "declaration": "*1:15:43",
      "variable_type": 0,
      "all_uses": ["*1:15:43"]
    }, {
      "id": 1,
      "usr": "c:type_usage_as_template_parameter_complex.cc@1062@F@empty#@local",
      "short_name": "local",
      "qualified_name": "local",
      "definition": "*1:54:39",
      "variable_type": 0,
      "all_uses": ["*1:54:39"]
    }]
}
*/