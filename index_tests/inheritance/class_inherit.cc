class Parent {};
class Derived : public Parent {};

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 3866412049634585509,
      "detailed_name": "class Parent {}",
      "qual_name_offset": 6,
      "short_name": "Parent",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:13|0|1|2|-1",
      "extent": "1:1-1:16|0|1|0|-1",
      "alias_of": 0,
      "bases": [],
      "derived": [10963370434658308541],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["2:24-2:30|10963370434658308541|2|2052|-1"]
    }, {
      "usr": 10963370434658308541,
      "detailed_name": "class Derived : public Parent {}",
      "qual_name_offset": 6,
      "short_name": "Derived",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:14|0|1|2|-1",
      "extent": "2:1-2:33|0|1|0|-1",
      "alias_of": 0,
      "bases": [3866412049634585509],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
