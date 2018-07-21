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
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:11|0|1|2",
      "extent": "1:1-1:14|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [11863524815063131483, 14022569716337624303],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:24-2:28|11863524815063131483|2|2052", "3:24-3:28|14022569716337624303|2|2052"]
    }, {
      "usr": 10963370434658308541,
      "detailed_name": "class Derived : public MiddleA, public MiddleB {}",
      "qual_name_offset": 6,
      "short_name": "Derived",
      "kind": 5,
      "declarations": [],
      "spell": "4:7-4:14|0|1|2",
      "extent": "4:1-4:50|0|1|0",
      "alias_of": 0,
      "bases": [11863524815063131483, 14022569716337624303],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 11863524815063131483,
      "detailed_name": "class MiddleA : public Root {}",
      "qual_name_offset": 6,
      "short_name": "MiddleA",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:14|0|1|2",
      "extent": "2:1-2:31|0|1|0",
      "alias_of": 0,
      "bases": [3897841498936210886],
      "derived": [10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["4:24-4:31|10963370434658308541|2|2052"]
    }, {
      "usr": 14022569716337624303,
      "detailed_name": "class MiddleB : public Root {}",
      "qual_name_offset": 6,
      "short_name": "MiddleB",
      "kind": 5,
      "declarations": [],
      "spell": "3:7-3:14|0|1|2",
      "extent": "3:1-3:31|0|1|0",
      "alias_of": 0,
      "bases": [3897841498936210886],
      "derived": [10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["4:40-4:47|10963370434658308541|2|2052"]
    }],
  "usr2var": []
}
*/
