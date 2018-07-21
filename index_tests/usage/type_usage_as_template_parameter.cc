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
      "kind": 12,
      "storage": 0,
      "declarations": [],
      "spell": "9:16-9:27|0|1|2",
      "extent": "9:1-12:2|0|1|0",
      "bases": [],
      "derived": [],
      "vars": [3364438781074774169],
      "uses": [],
      "callees": []
    }],
  "usr2type": [{
      "usr": 3286534761799572592,
      "detailed_name": "class unique_ptr {}",
      "qual_name_offset": 6,
      "short_name": "unique_ptr",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:17|0|1|2",
      "extent": "2:1-2:20|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["6:8-6:18|0|1|4", "7:8-7:18|0|1|4", "9:1-9:11|0|1|4", "10:3-10:13|16359708726068806331|3|4"]
    }, {
      "usr": 4186953406371619898,
      "detailed_name": "unique_ptr",
      "qual_name_offset": 0,
      "short_name": "unique_ptr",
      "kind": 26,
      "declarations": [],
      "spell": "2:7-2:17|0|1|2",
      "extent": "1:1-2:20|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [18075066956054788088, 3364438781074774169],
      "uses": []
    }, {
      "usr": 4750332761459066907,
      "detailed_name": "struct S {}",
      "qual_name_offset": 7,
      "short_name": "S",
      "kind": 23,
      "declarations": [],
      "spell": "4:8-4:9|0|1|2",
      "extent": "4:1-4:12|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["7:19-7:20|0|1|4", "9:12-9:13|0|1|4", "10:14-10:15|16359708726068806331|3|4"]
    }, {
      "usr": 16848604152578034754,
      "detailed_name": "unique_ptr",
      "qual_name_offset": 0,
      "short_name": "unique_ptr",
      "kind": 26,
      "declarations": [],
      "spell": "2:7-2:17|0|1|2",
      "extent": "1:1-2:20|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [12857919739649552168],
      "uses": []
    }],
  "usr2var": [{
      "usr": 3364438781074774169,
      "detailed_name": "unique_ptr<S> *local",
      "qual_name_offset": 15,
      "short_name": "local",
      "declarations": [],
      "spell": "10:18-10:23|16359708726068806331|3|2",
      "extent": "10:3-10:23|16359708726068806331|3|0",
      "type": 4186953406371619898,
      "uses": [],
      "kind": 13,
      "storage": 0
    }, {
      "usr": 12857919739649552168,
      "detailed_name": "static unique_ptr<bool> f0",
      "qual_name_offset": 24,
      "short_name": "f0",
      "declarations": [],
      "spell": "6:25-6:27|0|1|2",
      "extent": "6:1-6:27|0|1|0",
      "type": 16848604152578034754,
      "uses": [],
      "kind": 13,
      "storage": 2
    }, {
      "usr": 18075066956054788088,
      "detailed_name": "static unique_ptr<S> f1",
      "qual_name_offset": 21,
      "short_name": "f1",
      "declarations": [],
      "spell": "7:22-7:24|0|1|2",
      "extent": "7:1-7:24|0|1|0",
      "type": 4186953406371619898,
      "uses": [],
      "kind": 13,
      "storage": 2
    }]
}
*/
