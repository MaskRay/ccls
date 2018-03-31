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
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 11628904180681204356,
      "detailed_name": "Base0",
      "short_name": "Base0",
      "kind": 23,
      "declarations": ["2:12-2:17|-1|1|4", "7:18-7:23|-1|1|4"],
      "spell": "1:8-1:13|-1|1|2",
      "extent": "1:1-3:2|-1|1|0",
      "bases": [],
      "derived": [2],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["7:18-7:23|-1|1|4"]
    }, {
      "id": 1,
      "usr": 15826803741381445676,
      "detailed_name": "Base1",
      "short_name": "Base1",
      "kind": 23,
      "declarations": ["5:12-5:17|-1|1|4", "7:25-7:30|-1|1|4"],
      "spell": "4:8-4:13|-1|1|2",
      "extent": "4:1-6:2|-1|1|0",
      "bases": [],
      "derived": [2],
      "types": [],
      "funcs": [1],
      "vars": [],
      "instances": [],
      "uses": ["7:25-7:30|-1|1|4"]
    }, {
      "id": 2,
      "usr": 10963370434658308541,
      "detailed_name": "Derived",
      "short_name": "Derived",
      "kind": 23,
      "declarations": ["8:4-8:11|-1|1|4"],
      "spell": "7:8-7:15|-1|1|2",
      "extent": "7:1-9:2|-1|1|0",
      "bases": [0, 1],
      "derived": [],
      "types": [],
      "funcs": [2],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 16347272523198263017,
      "detailed_name": "void Base0::~Base0() noexcept",
      "short_name": "~Base0",
      "kind": 6,
      "storage": 1,
      "declarations": [],
      "spell": "2:11-2:17|0|2|2",
      "extent": "2:3-2:23|0|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [2],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 1,
      "usr": 8401779086123965305,
      "detailed_name": "void Base1::~Base1() noexcept",
      "short_name": "~Base1",
      "kind": 6,
      "storage": 1,
      "declarations": [],
      "spell": "5:11-5:17|1|2|2",
      "extent": "5:3-5:23|1|2|0",
      "declaring_type": 1,
      "bases": [],
      "derived": [2],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 2,
      "usr": 13164726294460837993,
      "detailed_name": "void Derived::~Derived() noexcept",
      "short_name": "~Derived",
      "kind": 6,
      "storage": 1,
      "declarations": [],
      "spell": "8:3-8:11|2|2|2",
      "extent": "8:3-8:26|2|2|0",
      "declaring_type": 2,
      "bases": [0, 1],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }],
  "vars": []
}
*/