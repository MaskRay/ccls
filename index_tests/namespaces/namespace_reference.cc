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
  "usr2func": [{
      "usr": 631910859630953711,
      "detailed_name": "void Runner()",
      "qual_name_offset": 5,
      "short_name": "Runner",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "6:6-6:12|0|1|2",
      "extent": "6:1-10:2|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": ["7:7-7:13|17328473273923617489|3|32", "9:3-9:9|17328473273923617489|3|32"]
    }, {
      "usr": 17328473273923617489,
      "detailed_name": "void ns::Accept(int a)",
      "qual_name_offset": 5,
      "short_name": "Accept",
      "kind": 12,
      "storage": 1,
      "declarations": [],
      "spell": "3:8-3:14|11072669167287398027|2|2",
      "extent": "3:3-3:24|11072669167287398027|2|0",
      "declaring_type": 11072669167287398027,
      "bases": [],
      "derived": [],
      "vars": [3649375698083002347],
      "uses": ["7:7-7:13|631910859630953711|3|32", "9:3-9:9|631910859630953711|3|32"],
      "callees": []
    }],
  "usr2type": [{
      "usr": 17,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [12898699035586282159, 3649375698083002347],
      "uses": []
    }, {
      "usr": 11072669167287398027,
      "detailed_name": "ns",
      "qual_name_offset": 0,
      "short_name": "ns",
      "kind": 3,
      "declarations": [],
      "spell": "1:11-1:13|0|1|2",
      "extent": "1:1-4:2|0|1|0",
      "alias_of": 0,
      "bases": [13838176792705659279],
      "derived": [],
      "types": [],
      "funcs": [17328473273923617489],
      "vars": [{
          "L": 12898699035586282159,
          "R": -1
        }],
      "instances": [],
      "uses": ["1:11-1:13|0|1|4", "7:3-7:5|631910859630953711|3|4", "7:14-7:16|631910859630953711|3|4", "8:19-8:21|631910859630953711|3|4"]
    }, {
      "usr": 13838176792705659279,
      "detailed_name": "",
      "qual_name_offset": 0,
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "alias_of": 0,
      "bases": [],
      "derived": [11072669167287398027],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }],
  "usr2var": [{
      "usr": 3649375698083002347,
      "detailed_name": "int a",
      "qual_name_offset": 4,
      "short_name": "a",
      "declarations": [],
      "spell": "3:19-3:20|17328473273923617489|3|2",
      "extent": "3:15-3:20|17328473273923617489|3|0",
      "type": 17,
      "uses": [],
      "kind": 253,
      "storage": 1
    }, {
      "usr": 12898699035586282159,
      "detailed_name": "int ns::Foo",
      "qual_name_offset": 4,
      "short_name": "Foo",
      "declarations": [],
      "spell": "2:7-2:10|11072669167287398027|2|2",
      "extent": "2:3-2:10|11072669167287398027|2|0",
      "type": 17,
      "uses": ["7:18-7:21|631910859630953711|3|4", "9:10-9:13|631910859630953711|3|4"],
      "kind": 13,
      "storage": 1
    }]
}
*/



