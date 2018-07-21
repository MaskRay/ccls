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
  "includes": [],
  "skipped_ranges": ["7:1-15:1", "17:1-33:1", "35:1-40:1", "42:1-53:1", "57:1-64:1", "68:1-79:1"],
  "usr2func": [{
      "usr": 1246637699196435450,
      "detailed_name": "unique_ptr<unique_ptr<S1, S2>, S2> *as_return_type(unique_ptr<S1, S2> *)",
      "qual_name_offset": 36,
      "short_name": "as_return_type",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "33:37-33:51|0|1|2",
      "extent": "33:1-33:92|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 13067214284561914253,
      "detailed_name": "void no_return_type(int)",
      "qual_name_offset": 5,
      "short_name": "no_return_type",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "40:6-40:20|0|1|2",
      "extent": "40:1-40:28|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 17922201480358737771,
      "detailed_name": "unique_ptr<S1, S2> *Foo::foo()",
      "qual_name_offset": 20,
      "short_name": "foo",
      "kind": 6,
      "storage": 0,
      "declarations": ["65:23-65:26|15041163540773201510|2|1025"],
      "spell": "79:26-79:29|15041163540773201510|2|1026",
      "extent": "79:1-79:51|15041163540773201510|2|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 18320186404467436976,
      "detailed_name": "void empty()",
      "qual_name_offset": 5,
      "short_name": "empty",
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "53:6-53:11|0|1|2",
      "extent": "53:1-55:2|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [500112618220246],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 4310164820010458371,
      "detailed_name": "struct S1",
      "qual_name_offset": 7,
      "short_name": "S1",
      "kind": 23,
      "declarations": ["4:8-4:10|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["15:30-15:32|0|1|4", "33:23-33:25|0|1|4", "33:63-33:65|0|1|4", "54:25-54:27|18320186404467436976|3|4", "65:14-65:16|15041163540773201510|2|4", "79:12-79:14|0|1|4"]
    }, {
      "usr": 7147635971744144194,
      "detailed_name": "template<> class unique_ptr<S1, S2>",
      "qual_name_offset": 17,
      "short_name": "unique_ptr",
      "kind": 5,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["15:19-15:29|0|1|4", "33:12-33:22|0|1|4", "33:52-33:62|0|1|4", "54:14-54:24|18320186404467436976|3|4", "65:3-65:13|15041163540773201510|2|4", "79:1-79:11|0|1|4"]
    }, {
      "usr": 12728490517004312484,
      "detailed_name": "struct S2",
      "qual_name_offset": 7,
      "short_name": "S2",
      "kind": 23,
      "declarations": ["5:8-5:10|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["15:34-15:36|0|1|4", "15:39-15:41|0|1|4", "33:27-33:29|0|1|4", "33:32-33:34|0|1|4", "33:67-33:69|0|1|4", "54:29-54:31|18320186404467436976|3|4", "54:34-54:36|18320186404467436976|3|4", "65:18-65:20|15041163540773201510|2|4", "79:16-79:18|0|1|4"]
    }, {
      "usr": 14209198335088845323,
      "detailed_name": "class unique_ptr",
      "qual_name_offset": 6,
      "short_name": "unique_ptr",
      "kind": 5,
      "declarations": ["2:7-2:17|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "class Foo {}",
      "qual_name_offset": 6,
      "short_name": "Foo",
      "kind": 5,
      "declarations": [],
      "spell": "64:7-64:10|0|1|2",
      "extent": "64:1-66:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [17922201480358737771],
      "vars": [],
      "instances": [],
      "uses": ["79:21-79:24|0|1|4"]
    }, {
      "usr": 18153735331422331128,
      "detailed_name": "unique_ptr",
      "qual_name_offset": 0,
      "short_name": "unique_ptr",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:17|0|1|2",
      "extent": "1:1-2:17|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [2933643612409209903, 500112618220246],
      "uses": ["15:8-15:18|0|1|4", "33:1-33:11|0|1|4", "54:3-54:13|18320186404467436976|3|4"]
    }],
  "usr2var": [{
      "usr": 500112618220246,
      "detailed_name": "unique_ptr<unique_ptr<S1, S2>, S2> *local",
      "qual_name_offset": 36,
      "short_name": "local",
      "declarations": [],
      "spell": "54:39-54:44|18320186404467436976|3|2",
      "extent": "54:3-54:44|18320186404467436976|3|0",
      "type": 18153735331422331128,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 2933643612409209903,
      "detailed_name": "extern unique_ptr<unique_ptr<S1, S2>, S2> f",
      "qual_name_offset": 42,
      "short_name": "f",
      "declarations": ["15:43-15:44|0|1|1"],
      "type": 18153735331422331128,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
