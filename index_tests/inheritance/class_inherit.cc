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
      "spell": "1:7-1:13|1:1-1:16|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [10963370434658308541],
      "instances": [],
      "uses": ["2:24-2:30|2052|-1"]
    }, {
      "usr": 10963370434658308541,
      "detailed_name": "class Derived : public Parent {}",
      "qual_name_offset": 6,
      "short_name": "Derived",
      "spell": "2:7-2:14|2:1-2:33|2|-1",
      "bases": [3866412049634585509],
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
    }],
  "usr2var": []
}
*/
