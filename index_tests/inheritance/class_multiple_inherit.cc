class Root {};
class MiddleA : public Root {};
class MiddleB : public Root {};
class Derived : public MiddleA, public MiddleB {};

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 3897841498936210886,
      "detailed_name": "class Root {}",
      "qual_name_offset": 6,
      "short_name": "Root",
      "spell": "1:7-1:11|1:1-1:14|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [11863524815063131483, 14022569716337624303],
      "instances": [],
      "uses": ["2:24-2:28|2052|-1", "3:24-3:28|2052|-1"]
    }, {
      "usr": 10963370434658308541,
      "detailed_name": "class Derived : public MiddleA, public MiddleB {}",
      "qual_name_offset": 6,
      "short_name": "Derived",
      "spell": "4:7-4:14|4:1-4:50|2|-1",
      "bases": [11863524815063131483, 14022569716337624303],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 11863524815063131483,
      "detailed_name": "class MiddleA : public Root {}",
      "qual_name_offset": 6,
      "short_name": "MiddleA",
      "spell": "2:7-2:14|2:1-2:31|2|-1",
      "bases": [3897841498936210886],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [10963370434658308541],
      "instances": [],
      "uses": ["4:24-4:31|2052|-1"]
    }, {
      "usr": 14022569716337624303,
      "detailed_name": "class MiddleB : public Root {}",
      "qual_name_offset": 6,
      "short_name": "MiddleB",
      "spell": "3:7-3:14|3:1-3:31|2|-1",
      "bases": [3897841498936210886],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [10963370434658308541],
      "instances": [],
      "uses": ["4:40-4:47|2052|-1"]
    }],
  "usr2var": []
}
*/
