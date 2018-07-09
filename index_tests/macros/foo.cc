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
      "kind": 9,
      "storage": 0,
      "declarations": [],
      "spell": "5:12-5:15|15041163540773201510|2|514",
      "extent": "1:1-1:1|15041163540773201510|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 53,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [10677751717622394455],
      "uses": []
    }, {
      "usr": 15041163540773201510,
      "detailed_name": "struct Foo {}",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "kind": 23,
      "declarations": [],
      "spell": "4:8-4:11|0|1|2",
      "extent": "4:1-6:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [13788753348312146871],
      "vars": [],
      "instances": [],
      "uses": ["5:12-5:15|0|1|4"]
    }],
  "usr2var": [{
      "usr": 1569772797058982873,
      "detailed_name": "A",
      "qual_name_offset": 0,
      "short_name": "A",
      "hover": "#define A 5",
      "declarations": [],
      "spell": "1:9-1:10|0|1|2",
      "extent": "1:9-1:12|0|1|0",
      "type": 0,
      "uses": ["8:9-8:10|0|1|4"],
      "kind": 255,
      "storage": 0
    }, {
      "usr": 4904139678698066671,
      "detailed_name": "DISALLOW",
      "qual_name_offset": 0,
      "short_name": "DISALLOW",
      "hover": "#define DISALLOW(type) type(type&&) = delete;",
      "declarations": [],
      "spell": "2:9-2:17|0|1|2",
      "extent": "2:9-2:46|0|1|0",
      "type": 0,
      "uses": ["5:3-5:16|0|1|4"],
      "kind": 255,
      "storage": 0
    }, {
      "usr": 10677751717622394455,
      "detailed_name": "int x",
      "qual_name_offset": 4,
      "short_name": "x",
      "hover": "int x = ",
      "declarations": [],
      "spell": "8:5-8:6|0|1|2",
      "extent": "8:1-1:1|0|1|0",
      "type": 53,
      "uses": [],
      "kind": 13,
      "storage": 0
    }]
}
*/
