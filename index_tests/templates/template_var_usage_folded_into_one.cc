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
  "skipped_ranges": ["12:1-29:1"],
  "usr2func": [],
  "usr2type": [{
      "usr": 6697181287623958829,
      "detailed_name": "enum A {}",
      "qual_name_offset": 5,
      "short_name": "A",
      "spell": "1:6-1:7|1:1-1:10|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 10,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [16721564935990383768],
      "uses": ["7:1-7:2|4|-1", "7:11-7:12|4|-1"]
    }, {
      "usr": 11919899838872947844,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "T",
      "spell": "4:19-4:20|4:10-4:20|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 26,
      "parent_kind": 5,
      "declarations": [],
      "derived": [],
      "instances": [8096973118640070624],
      "uses": []
    }, {
      "usr": 13892793056005362145,
      "detailed_name": "enum B {}",
      "qual_name_offset": 5,
      "short_name": "B",
      "spell": "2:6-2:7|2:1-2:10|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 10,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [12028309045033782423],
      "uses": ["8:1-8:2|4|-1", "8:11-8:12|4|-1"]
    }],
  "usr2var": [{
      "usr": 8096973118640070624,
      "detailed_name": "T var",
      "qual_name_offset": 2,
      "short_name": "var",
      "hover": "T var = T()",
      "spell": "5:3-5:6|5:1-5:12|2|-1",
      "type": 11919899838872947844,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": ["7:7-7:10|12|-1", "8:7-8:10|12|-1"]
    }, {
      "usr": 12028309045033782423,
      "detailed_name": "B b",
      "qual_name_offset": 2,
      "short_name": "b",
      "hover": "B b = var<B>",
      "spell": "8:3-8:4|8:1-8:13|2|-1",
      "type": 13892793056005362145,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 16721564935990383768,
      "detailed_name": "A a",
      "qual_name_offset": 2,
      "short_name": "a",
      "hover": "A a = var<A>",
      "spell": "7:3-7:4|7:1-7:13|2|-1",
      "type": 6697181287623958829,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
