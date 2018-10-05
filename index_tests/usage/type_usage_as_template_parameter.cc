template<typename T>
class unique_ptr {};

struct S {};

static unique_ptr<bool> f0;
static unique_ptr<S> f1;

unique_ptr<S>* return_type() {
  unique_ptr<S>* local;
  return nullptr;
}
/*
OUTPUT:
{
  "includes": [],
  "skipped_ranges": [],
  "usr2func": [{
      "usr": 16359708726068806331,
      "detailed_name": "unique_ptr<S> *return_type()",
      "qual_name_offset": 15,
      "short_name": "return_type",
      "spell": "9:16-9:27|9:1-12:2|2|-1",
      "bases": [],
      "vars": [3364438781074774169],
      "callees": [],
      "kind": 12,
      "parent_kind": 0,
      "storage": 0,
      "declarations": [],
      "derived": [],
      "uses": []
    }],
  "usr2type": [{
      "usr": 3286534761799572592,
      "detailed_name": "class unique_ptr {}",
      "qual_name_offset": 6,
      "short_name": "unique_ptr",
      "spell": "2:7-2:17|2:1-2:20|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 5,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [12857919739649552168, 18075066956054788088, 3364438781074774169],
      "uses": ["6:8-6:18|4|-1", "7:8-7:18|4|-1", "9:1-9:11|4|-1", "10:3-10:13|4|-1"]
    }, {
      "usr": 4750332761459066907,
      "detailed_name": "struct S {}",
      "qual_name_offset": 7,
      "short_name": "S",
      "spell": "4:8-4:9|4:1-4:12|2|-1",
      "bases": [],
      "funcs": [],
      "types": [],
      "vars": [],
      "alias_of": 0,
      "kind": 23,
      "parent_kind": 0,
      "declarations": [],
      "derived": [],
      "instances": [],
      "uses": ["7:19-7:20|4|-1", "9:12-9:13|4|-1", "10:14-10:15|4|-1"]
    }],
  "usr2var": [{
      "usr": 3364438781074774169,
      "detailed_name": "unique_ptr<S> *local",
      "qual_name_offset": 15,
      "short_name": "local",
      "spell": "10:18-10:23|10:3-10:23|2|-1",
      "type": 3286534761799572592,
      "kind": 13,
      "parent_kind": 12,
      "storage": 0,
      "declarations": [],
      "uses": []
    }, {
      "usr": 12857919739649552168,
      "detailed_name": "static unique_ptr<bool> f0",
      "qual_name_offset": 24,
      "short_name": "f0",
      "spell": "6:25-6:27|6:1-6:27|2|-1",
      "type": 3286534761799572592,
      "kind": 13,
      "parent_kind": 0,
      "storage": 2,
      "declarations": [],
      "uses": []
    }, {
      "usr": 18075066956054788088,
      "detailed_name": "static unique_ptr<S> f1",
      "qual_name_offset": 21,
      "short_name": "f1",
      "spell": "7:22-7:24|7:1-7:24|2|-1",
      "type": 3286534761799572592,
      "kind": 13,
      "parent_kind": 0,
      "storage": 2,
      "declarations": [],
      "uses": []
    }]
}
*/
