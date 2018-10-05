struct Base0 {
  virtual ~Base0() { }
};
struct Base1 {
  virtual ~Base1() { }
};
struct Derived : Base0, Base1 {
  ~Derived() override { }
};

/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 8401779086123965305,
      "detailed_name": "virtual Base1::~Base1() noexcept",
      "qual_name_offset": 8,
      "short_name": "~Base1",
      "spell": "5:11-5:17|5:3-5:23|1090|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 23,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 13164726294460837993,
      "detailed_name": "Derived::~Derived() noexcept",
      "qual_name_offset": 0,
      "short_name": "~Derived",
      "spell": "8:3-8:11|8:3-8:26|5186|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 23,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }, {
      "usr": 16347272523198263017,
      "detailed_name": "virtual Base0::~Base0() noexcept",
      "qual_name_offset": 8,
      "short_name": "~Base0",
      "spell": "2:11-2:17|2:3-2:23|1090|-1",
      "bases": [],
      "vars": [],
      "callees": [],
      "kind": 6,
      "parent_kind": 23,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 10963370434658308541,
      "detailed_name": "struct Derived : Base0, Base1 {}",
      "qual_name_offset": 7,
      "short_name": "Derived",
      "spell": "7:8-7:15|7:1-9:2|2|-1",
      "bases": [11628904180681204356, 15826803741381445676],
      "funcs": [13164726294460837993],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["8:4-8:11|4|-1"]
    }, {
      "usr": 11628904180681204356,
      "detailed_name": "struct Base0 {}",
      "qual_name_offset": 7,
      "short_name": "Base0",
      "spell": "1:8-1:13|1:1-3:2|2|-1",
      "bases": [],
      "funcs": [16347272523198263017],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [10963370434658308541],
      "instances": [],
      "uses": ["2:12-2:17|4|-1", "7:18-7:23|2052|-1"]
    }, {
      "usr": 15826803741381445676,
      "detailed_name": "struct Base1 {}",
      "qual_name_offset": 7,
      "short_name": "Base1",
      "spell": "4:8-4:13|4:1-6:2|2|-1",
      "bases": [],
      "funcs": [8401779086123965305],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [10963370434658308541],
      "instances": [],
      "uses": ["5:12-5:17|4|-1", "7:25-7:30|2052|-1"]
    }],
  "usr2var": []
}
*/