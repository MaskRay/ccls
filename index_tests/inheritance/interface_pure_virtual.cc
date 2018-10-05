class IFoo {
  virtual void foo() = 0;
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 3277829753446788562,
      "detailed_name": "virtual void IFoo::foo() = 0",
      "qual_name_offset": 13,
      "short_name": "foo",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 0,
      "storage": 0,
      "declarations": ["2:16-2:19|2:3-2:25|1089|-1"],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 9949214233977131946,
      "detailed_name": "class IFoo {}",
      "qual_name_offset": 6,
      "short_name": "IFoo",
      "spell": "1:7-1:11|1:1-3:2|2|-1",
      "bases": [],
      "funcs": [3277829753446788562],
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
