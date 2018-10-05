template<typename T>
struct Foo;

using Foo1 = Foo<int>;
typedef Foo<Foo1> Foo2;

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 1544499294580512394,
      "detailed_name": "using Foo1 = Foo<int>",
      "qual_name_offset": 6,
      "short_name": "Foo1",
      "spell": "4:7-4:11|4:1-4:22|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 10528472276654770367,
      "kind": 252,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["5:13-5:17|4|-1"]
    }, {
      "usr": 10528472276654770367,
      "detailed_name": "struct Foo",
      "qual_name_offset": 7,
      "short_name": "Foo",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": ["2:8-2:11|2:1-2:11|1|-1"],
      "derived": [],
      "instances": [],
      "uses": ["4:14-4:17|4|-1", "5:9-5:12|4|-1"]
    }, {
      "usr": 15933698173231330933,
      "detailed_name": "typedef Foo<Foo1> Foo2",
      "qual_name_offset": 18,
      "short_name": "Foo2",
      "spell": "5:19-5:23|5:1-5:23|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 10528472276654770367,
      "kind": 252,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
