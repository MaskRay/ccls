class Root {};
class MiddleA : public Root {};
class MiddleB : public Root {};
class Derived : public MiddleA, public MiddleB {};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 3897841498936210886,
      "detailed_name": "Root",
      "short_name": "Root",
      "kind": 5,
      "declarations": ["2:24-2:28|-1|1|4", "3:24-3:28|-1|1|4"],
      "spell": "1:7-1:11|-1|1|2",
      "extent": "1:1-1:14|-1|1|0",
      "bases": [],
      "derived": [1, 2],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:24-2:28|-1|1|4", "3:24-3:28|-1|1|4"]
    }, {
      "id": 1,
      "usr": 11863524815063131483,
      "detailed_name": "MiddleA",
      "short_name": "MiddleA",
      "kind": 5,
      "declarations": ["4:24-4:31|-1|1|4"],
      "spell": "2:7-2:14|-1|1|2",
      "extent": "2:1-2:31|-1|1|0",
      "bases": [0],
      "derived": [3],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["4:24-4:31|-1|1|4"]
    }, {
      "id": 2,
      "usr": 14022569716337624303,
      "detailed_name": "MiddleB",
      "short_name": "MiddleB",
      "kind": 5,
      "declarations": ["4:40-4:47|-1|1|4"],
      "spell": "3:7-3:14|-1|1|2",
      "extent": "3:1-3:31|-1|1|0",
      "bases": [0],
      "derived": [3],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["4:40-4:47|-1|1|4"]
    }, {
      "id": 3,
      "usr": 10963370434658308541,
      "detailed_name": "Derived",
      "short_name": "Derived",
      "kind": 5,
      "declarations": [],
      "spell": "4:7-4:14|-1|1|2",
      "extent": "4:1-4:50|-1|1|0",
      "bases": [1, 2],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "funcs": [],
  "vars": []
}
*/
