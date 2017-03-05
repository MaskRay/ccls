enum A {};
enum B {};

template<typename T>
T var = 3;

int a = var<A>;
int b = var<B>;

// TODO: No usages of types on var.
//       libclang doesn't expose the info. File a bug.

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
      "definition": "1:1:6",
      "uses": ["*1:1:6", "1:7:13"]
    }, {
      "id": 1,
      "usr": "c:@E@B",
      "short_name": "B",
      "qualified_name": "B",
      "definition": "1:2:6",
      "uses": ["*1:2:6", "1:8:13"]
    }],
  "functions": [],
  "variables": [{
      "id": 0,
      "usr": "c:@var",
      "short_name": "var",
      "qualified_name": "var",
      "definition": "1:5:3",
      "uses": ["1:5:3"]
    }, {
      "id": 1,
      "usr": "c:@a",
      "short_name": "a",
      "qualified_name": "a",
      "definition": "1:7:5",
      "uses": ["1:7:5"]
    }, {
      "id": 2,
      "usr": "c:@b",
      "short_name": "b",
      "qualified_name": "b",
      "definition": "1:8:5",
      "uses": ["1:8:5"]
    }]
}
*/