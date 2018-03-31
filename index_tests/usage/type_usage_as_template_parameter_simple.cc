template<typename T>
class unique_ptr {};

struct S;

static unique_ptr<S> foo;

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 3286534761799572592,
      "detailed_name": "unique_ptr",
      "short_name": "unique_ptr",
      "kind": 5,
      "declarations": [],
      "spell": "2:7-2:17|-1|1|2",
      "extent": "2:1-2:20|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["6:8-6:18|-1|1|4"]
    }, {
      "id": 1,
      "usr": 4750332761459066907,
      "detailed_name": "S",
      "short_name": "S",
      "kind": 23,
      "declarations": ["4:8-4:9|-1|1|1"],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["6:19-6:20|-1|1|4"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 3398408600781120939,
      "detailed_name": "unique_ptr<S> foo",
      "short_name": "foo",
      "declarations": [],
      "spell": "6:22-6:25|-1|1|2",
      "extent": "6:1-6:25|-1|1|0",
      "type": 0,
      "uses": [],
      "kind": 13,
      "storage": 3
    }]
}
*/
