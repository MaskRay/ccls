class Parent {};
class Derived : public Parent {};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 3866412049634585509,
      "detailed_name": "Parent",
      "short_name": "Parent",
      "kind": 5,
      "declarations": ["2:24-2:30|-1|1|4"],
      "spell": "1:7-1:13|-1|1|2",
      "extent": "1:1-1:16|-1|1|0",
      "bases": [],
      "derived": [1],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:24-2:30|-1|1|4"]
    }, {
      "id": 1,
      "usr": 10963370434658308541,
      "detailed_name": "Derived",
      "short_name": "Derived",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:14|-1|1|2",
      "extent": "2:1-2:33|-1|1|0",
      "bases": [0],
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
