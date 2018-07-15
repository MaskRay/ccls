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
      "kind": 6,
      "storage": 0,
      "declarations": ["2:16-2:19|9949214233977131946|2|1089"],
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 9949214233977131946,
      "detailed_name": "class IFoo {}",
      "qual_name_offset": 6,
      "short_name": "IFoo",
      "kind": 5,
      "declarations": [],
      "spell": "1:7-1:11|0|1|2",
      "extent": "1:1-3:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [3277829753446788562],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": []
}
*/
