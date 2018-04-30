class Root {};
class MiddleA : public Root {};
class MiddleB : public Root {};
class Derived : public MiddleA, public MiddleB {};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 3897841498936210886,
      "detailed_name": "Root",
      "qual_name_offset": 0,
      "short_name": "Root",
      "kind": 5,
      "declarations": ["2:24-2:28|0|1|4", "3:24-3:28|0|1|4"],
      "spell": "1:7-1:11|0|1|2",
      "extent": "1:1-1:14|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [11863524815063131483, 14022569716337624303],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:24-2:28|0|1|4", "3:24-3:28|0|1|4"]
    }, {
      "usr": 10963370434658308541,
      "detailed_name": "Derived",
      "qual_name_offset": 0,
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
      "detailed_name": "MiddleA",
      "qual_name_offset": 0,
      "short_name": "MiddleA",
      "kind": 5,
      "declarations": ["4:24-4:31|0|1|4"],
      "spell": "2:7-2:14|0|1|2",
      "extent": "2:1-2:31|0|1|0",
      "alias_of": 0,
      "bases": [3897841498936210886],
      "derived": [10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["4:24-4:31|0|1|4"]
    }, {
      "usr": 14022569716337624303,
      "detailed_name": "MiddleB",
      "qual_name_offset": 0,
      "short_name": "MiddleB",
      "kind": 5,
      "declarations": ["4:40-4:47|0|1|4"],
      "spell": "3:7-3:14|0|1|2",
      "extent": "3:1-3:31|0|1|0",
      "alias_of": 0,
      "bases": [3897841498936210886],
      "derived": [10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["4:40-4:47|0|1|4"]
    }],
  "usr2var": []
}
*/
