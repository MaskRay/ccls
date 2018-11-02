#define A 5
#define DISALLOW(type) type(type&&) = delete;

struct Foo {
  DISALLOW(Foo);
};

int x = A;

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 13788753348312146871,
      "detailed_name": "Foo::Foo(Foo &&) = delete",
      "qual_name_offset": 0,
      "short_name": "Foo",
      "spell": "5:12-5:15|5:3-5:11|1026|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 9,
      "parent_kind": 23,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": ["5:12-5:15|64|0"]
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 0,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [10677751717622394455],
      "uses": []
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "spell": "4:8-4:11|4:1-6:2|2|-1",
      "bases": [],
      "funcs": [13788753348312146871],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["5:12-5:15|4|-1", "5:12-5:15|64|0"]
    }],
  "usr2var": [{
      "usr": 1569772797058982873,
      "detailed_name": "A",
      "qual_name_offset": 0,
      "short_name": "A",
      "hover": "#define A 5",
      "spell": "1:9-1:10|1:9-1:12|2|-1",
      "type": 0,
      "kind": 255,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "uses": ["8:9-8:10|64|-1"]
    }, {
      "usr": 4904139678698066671,
      "detailed_name": "DISALLOW",
      "qual_name_offset": 0,
      "short_name": "DISALLOW",
      "hover": "#define DISALLOW(type) type(type&&) = delete;",
      "spell": "2:9-2:17|2:9-2:46|2|-1",
      "type": 0,
      "kind": 255,
      "parent_kind": 1,
      "storage": 0,
      "declarations": [],
      "uses": ["5:3-5:11|64|-1"]
    }, {
      "usr": 10677751717622394455,
      "detailed_name": "int x",
      "qual_name_offset": 4,
      "short_name": "x",
      "hover": "int x = A",
      "spell": "8:5-8:6|8:1-8:10|2|-1",
      "type": 53,
      "kind": 13,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "uses": []
    }]
}
*/
