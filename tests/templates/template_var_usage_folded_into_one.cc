enum A {};
enum B {};

template<typename T>
T var = 3;

int a = var<A>;
int b = var<B>;

// NOTE: libclang before 4.0 doesn't expose template usage on |var|.

#if false
EnumDecl A
EnumDecl B
UnexposedDecl var
VarDecl a
  UnexposedExpr var
    UnexposedExpr var
      DeclRefExpr var
        TypeRef enum A
UnexposedDecl var
VarDecl b
  UnexposedExpr var
    UnexposedExpr var
      DeclRefExpr var
        TypeRef enum B
UnexposedDecl var
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
      "uses": ["*1:6-1:7", "7:13-7:14"]
    }, {
      "id": 1,
      "usr": "c:@E@B",
      "short_name": "B",
      "qualified_name": "B",
      "definition_spelling": "2:6-2:7",
      "definition_extent": "2:1-2:10",
      "uses": ["*2:6-2:7", "8:13-8:14"]
    }, {
      "id": 2,
      "usr": "c:template_var_usage_folded_into_one.cc@35",
      "uses": ["*5:1-5:2"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@VT>1#T@var",
      "short_name": "var",
      "qualified_name": "T var",
      "definition_spelling": "5:3-5:6",
      "definition_extent": "5:1-5:10",
      "uses": ["5:3-5:6", "7:9-7:12", "8:9-8:12"]
    }, {
      "id": 1,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "int a",
      "definition_spelling": "7:5-7:6",
      "definition_extent": "7:1-7:15",
      "uses": ["7:5-7:6"]
    }, {
      "id": 2,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "int b",
      "definition_spelling": "8:5-8:6",
      "definition_extent": "8:1-8:15",
      "uses": ["8:5-8:6"]
    }]
}
*/
