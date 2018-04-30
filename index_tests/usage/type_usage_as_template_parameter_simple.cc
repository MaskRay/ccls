template<typename T>
class unique_ptr {};

struct S;

static unique_ptr<S> foo;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "usr2func": [],
  "usr2type": [{
      "usr": 3286534761799572592,
      "detailed_name": "unique_ptr",
      "qual_name_offset": 0,
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
      "instances": [3398408600781120939],
      "uses": ["6:8-6:18|0|1|4"]
    }, {
      "usr": 4750332761459066907,
      "detailed_name": "S",
      "qual_name_offset": 0,
      "short_name": "S",
      "kind": 23,
      "declarations": ["4:8-4:9|0|1|1"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["6:19-6:20|0|1|4"]
    }],
  "usr2var": [{
      "usr": 3398408600781120939,
      "detailed_name": "unique_ptr<S> foo",
      "qual_name_offset": 14,
      "short_name": "foo",
      "declarations": [],
      "spell": "6:22-6:25|0|1|2",
      "extent": "6:1-6:25|0|1|0",
      "type": 3286534761799572592,
      "uses": [],
      "kind": 13,
      "storage": 3
    }]
}
*/
