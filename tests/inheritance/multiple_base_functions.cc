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
      "usr": "c:@S@Base0",
      "short_name": "Base0",
      "detailed_name": "Base0",
      "definition_spelling": "1:8-1:13",
      "definition_extent": "1:1-3:2",
      "parents": [],
      "derived": [2],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [],
      "uses": ["1:8-1:13", "2:12-2:17", "7:18-7:23"]
    }, {
      "id": 1,
      "usr": "c:@S@Base1",
      "short_name": "Base1",
      "detailed_name": "Base1",
      "definition_spelling": "4:8-4:13",
      "definition_extent": "4:1-6:2",
      "parents": [],
      "derived": [2],
      "types": [],
      "funcs": [1],
      "vars": [],
      "instances": [],
      "uses": ["4:8-4:13", "5:12-5:17", "7:25-7:30"]
    }, {
      "id": 2,
      "usr": "c:@S@Derived",
      "short_name": "Derived",
      "detailed_name": "Derived",
      "definition_spelling": "7:8-7:15",
      "definition_extent": "7:1-9:2",
      "parents": [0, 1],
      "derived": [],
      "types": [],
      "funcs": [2],
      "vars": [],
      "instances": [],
      "uses": ["7:8-7:15", "8:4-8:11"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": false,
      "usr": "c:@S@Base0@F@~Base0#",
      "short_name": "~Base0",
      "detailed_name": "void Base0::~Base0() noexcept",
      "declarations": [],
      "definition_spelling": "2:11-2:17",
      "definition_extent": "2:3-2:23",
      "declaring_type": 0,
      "base": [],
      "derived": [2],
      "locals": [],
      "callers": [],
      "callees": []
    }, {
      "id": 1,
      "is_operator": false,
      "usr": "c:@S@Base1@F@~Base1#",
      "short_name": "~Base1",
      "detailed_name": "void Base1::~Base1() noexcept",
      "declarations": [],
      "definition_spelling": "5:11-5:17",
      "definition_extent": "5:3-5:23",
      "declaring_type": 1,
      "base": [],
      "derived": [2],
      "locals": [],
      "callers": [],
      "callees": []
    }, {
      "id": 2,
      "is_operator": false,
      "usr": "c:@S@Derived@F@~Derived#",
      "short_name": "~Derived",
      "detailed_name": "void Derived::~Derived() noexcept",
      "declarations": [],
      "definition_spelling": "8:3-8:11",
      "definition_extent": "8:3-8:26",
      "declaring_type": 2,
      "base": [0, 1],
      "derived": [],
      "locals": [],
      "callers": [],
      "callees": []
    }],
  "vars": []
}
*/