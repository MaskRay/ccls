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
      "instantiations": [0, 1],
      "uses": ["2:7-2:17", "*15:8-15:18", "*15:19-15:29", "*33:1-33:11", "*33:12-33:22", "*33:52-33:62", "*54:3-54:13", "*54:14-54:24", "*65:3-65:13", "*79:1-79:11"]
    }, {
      "id": 1,
      "usr": "c:@S@S1",
      "uses": ["4:8-4:10", "*15:30-15:32", "*33:23-33:25", "*33:63-33:65", "*54:25-54:27", "*65:14-65:16", "*79:12-79:14"]
    }, {
      "id": 2,
      "usr": "c:@S@S2",
      "uses": ["5:8-5:10", "*15:34-15:36", "*15:39-15:41", "*33:27-33:29", "*33:32-33:34", "*33:67-33:69", "*54:29-54:31", "*54:34-54:36", "*65:18-65:20", "*79:16-79:18"]
    }, {
      "id": 3,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "qualified_name": "Foo",
      "definition_spelling": "64:7-64:10",
      "definition_extent": "64:1-66:2",
      "funcs": [3],
      "uses": ["*64:7-64:10", "79:21-79:24"]
    }],
  "funcs": [{
      "id": 0,
      "usr": "c:@F@as_return_type#*$@S@unique_ptr>#$@S@S1#$@S@S2#",
      "short_name": "as_return_type",
      "qualified_name": "as_return_type",
      "hover": "unique_ptr<unique_ptr<S1, S2>, S2> *(unique_ptr<S1, S2> *)",
      "definition_spelling": "33:37-33:51",
      "definition_extent": "33:1-33:92"
    }, {
      "id": 1,
      "usr": "c:@F@no_return_type#I#",
      "short_name": "no_return_type",
      "qualified_name": "no_return_type",
      "hover": "void (int)",
      "definition_spelling": "40:6-40:20",
      "definition_extent": "40:1-40:28"
    }, {
      "id": 2,
      "usr": "c:@F@empty#",
      "short_name": "empty",
      "qualified_name": "empty",
      "hover": "void ()",
      "definition_spelling": "53:6-53:11",
      "definition_extent": "53:1-55:2"
    }, {
      "id": 3,
      "usr": "c:@S@Foo@F@foo#",
      "short_name": "foo",
      "qualified_name": "Foo::foo",
      "hover": "unique_ptr<S1, S2> *()",
      "declarations": ["65:23-65:26"],
      "definition_spelling": "79:26-79:29",
      "definition_extent": "79:1-79:51",
      "declaring_type": 3
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@f",
      "short_name": "f",
      "qualified_name": "f",
      "hover": "unique_ptr<unique_ptr<S1, S2>, S2>",
      "declaration": "15:43-15:44",
      "variable_type": 0,
      "uses": ["15:43-15:44"]
    }, {
      "id": 1,
      "usr": "c:type_usage_as_template_parameter_complex.cc@1062@F@empty#@local",
      "short_name": "local",
      "qualified_name": "local",
      "hover": "unique_ptr<unique_ptr<S1, S2>, S2> *",
      "definition_spelling": "54:39-54:44",
      "definition_extent": "54:3-54:44",
      "variable_type": 0,
      "uses": ["54:39-54:44"]
    }]
}
*/
