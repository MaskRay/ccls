enum A {};
enum B {};

template<typename T>
T var = T();

A a = var<A>;
B b = var<B>;

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
EXTRA_FLAGS:
-std=c++14

OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": ["12:1-28:7"],
  "types": [{
      "id": 0,
      "usr": 6697181287623958829,
      "detailed_name": "A",
      "short_name": "A",
      "kind": 10,
      "declarations": [],
      "spell": "1:6-1:7|-1|1|2",
      "extent": "1:1-1:10|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [1],
      "uses": ["7:1-7:2|-1|1|4", "7:11-7:12|-1|1|4"]
    }, {
      "id": 1,
      "usr": 13892793056005362145,
      "detailed_name": "B",
      "short_name": "B",
      "kind": 10,
      "declarations": [],
      "spell": "2:6-2:7|-1|1|2",
      "extent": "2:1-2:10|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [2],
      "uses": ["8:1-8:2|-1|1|4", "8:11-8:12|-1|1|4"]
    }, {
      "id": 2,
      "usr": 8864163146308556810,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["5:1-5:2|-1|1|4", "5:9-5:10|-1|1|4"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 8096973118640070624,
      "detailed_name": "T var",
      "short_name": "var",
      "declarations": [],
      "spell": "5:3-5:6|-1|1|2",
      "extent": "5:1-5:12|-1|1|0",
      "uses": ["7:7-7:10|-1|1|4", "8:7-8:10|-1|1|4"],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 16721564935990383768,
      "detailed_name": "A a",
      "short_name": "a",
      "hover": "A a = var<A>",
      "declarations": [],
      "spell": "7:3-7:4|-1|1|2",
      "extent": "7:1-7:13|-1|1|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 2,
      "usr": 12028309045033782423,
      "detailed_name": "B b",
      "short_name": "b",
      "hover": "B b = var<B>",
      "declarations": [],
      "spell": "8:3-8:4|-1|1|2",
      "extent": "8:1-8:13|-1|1|0",
      "type": 1,
      "uses": [],
      "kind": 13,
      "storage": 1
    }]
}
*/
