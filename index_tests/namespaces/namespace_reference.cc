namespace ns {
  int Foo;
  void Accept(int a) {}
}

void Runner() {
  ns::Accept(ns::Foo);
  using namespace ns;
  Accept(Foo);
}

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 11072669167287398027,
      "detailed_name": "ns",
      "short_name": "ns",
      "kind": 3,
      "declarations": [],
      "spell": "1:11-1:13|-1|1|2",
      "extent": "1:1-4:2|-1|1|0",
      "bases": [1],
      "derived": [],
      "types": [],
      "funcs": [0],
      "vars": [0],
      "instances": [],
      "uses": ["1:11-1:13|-1|1|4", "7:3-7:5|1|3|4", "7:14-7:16|1|3|4", "8:19-8:21|1|3|4"]
    }, {
      "id": 1,
      "usr": 13838176792705659279,
      "detailed_name": "<fundamental>",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [0],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "id": 2,
      "usr": 17,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0, 1],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 17328473273923617489,
      "detailed_name": "void ns::Accept(int a)",
      "short_name": "Accept",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "3:8-3:14|0|2|2",
      "extent": "3:3-3:24|0|2|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [1],
      "uses": ["7:7-7:13|1|3|32", "9:3-9:9|1|3|32"],
      "callees": []
    }, {
      "id": 1,
      "usr": 631910859630953711,
      "detailed_name": "void Runner()",
      "short_name": "Runner",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "6:6-6:12|-1|1|2",
      "extent": "6:1-10:2|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["7:7-7:13|0|3|32", "9:3-9:9|0|3|32"]
    }],
  "vars": [{
      "id": 0,
      "usr": 12898699035586282159,
      "detailed_name": "int ns::Foo",
      "short_name": "Foo",
      "declarations": [],
      "spell": "2:7-2:10|0|2|2",
      "extent": "2:3-2:10|0|2|0",
      "type": 2,
      "uses": ["7:18-7:21|1|3|4", "9:10-9:13|1|3|4"],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 7976909968919750794,
      "detailed_name": "int a",
      "short_name": "a",
      "declarations": [],
      "spell": "3:19-3:20|0|3|2",
      "extent": "3:15-3:20|0|3|0",
      "type": 2,
      "uses": [],
      "kind": 253,
      "storage": 1
    }]
}
*/



