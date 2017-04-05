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
      "definition": "1:6",
      "uses": ["*1:6", "7:13"]
    }, {
      "id": 1,
      "usr": "c:@E@B",
      "short_name": "B",
      "qualified_name": "B",
      "definition": "2:6",
      "uses": ["*2:6", "8:13"]
    }, {
      "id": 2,
      "usr": "c:template_var_usage_folded_into_one.cc@35",
      "uses": ["*5:1"]
    }],
  "vars": [{
      "id": 0,
      "usr": "c:@VT>1#T@var",
      "short_name": "var",
      "qualified_name": "var",
      "definition": "5:3",
      "uses": ["5:3", "7:9", "8:9"]
    }, {
      "id": 1,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "7:5",
      "uses": ["7:5"]
    }, {
      "id": 2,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition": "8:5",
      "uses": ["8:5"]
    }]
}
*/
