template<unsigned int C>
class Base1 {};

template<typename C>
class Base2 {};

template<unsigned int T>
class Derived1 : Base1<T> {};

template<typename T>
class Derived2 : Base2<T> {};

class Derived : Base1<3>, Base2<Derived>, Derived1<4>, Derived2<Derived> {};

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 11930058224338108382,
      "detailed_name": "Base1",
      "short_name": "Base1",
      "kind": 5,
      "declarations": ["8:18-8:23|-1|1|4", "13:17-13:22|-1|1|4"],
      "spell": "2:7-2:12|-1|1|2",
      "extent": "2:1-2:15|-1|1|0",
      "bases": [],
      "derived": [2, 6],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["8:18-8:23|-1|1|4", "13:17-13:22|-1|1|4"]
    }, {
      "id": 1,
      "usr": 11118288764693061434,
      "detailed_name": "Base2",
      "short_name": "Base2",
      "kind": 5,
      "declarations": ["11:18-11:23|-1|1|4", "13:27-13:32|-1|1|4"],
      "spell": "5:7-5:12|-1|1|2",
      "extent": "5:1-5:15|-1|1|0",
      "bases": [],
      "derived": [4, 6],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["11:18-11:23|-1|1|4", "13:27-13:32|-1|1|4"]
    }, {
      "id": 2,
      "usr": 5863733211528032190,
      "detailed_name": "Derived1",
      "short_name": "Derived1",
      "kind": 5,
      "declarations": ["13:43-13:51|-1|1|4"],
      "spell": "8:7-8:15|-1|1|2",
      "extent": "8:1-8:29|-1|1|0",
      "bases": [0],
      "derived": [6],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:43-13:51|-1|1|4"]
    }, {
      "id": 3,
      "usr": 9,
      "detailed_name": "",
      "short_name": "",
      "kind": 0,
      "declarations": [],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": []
    }, {
      "id": 4,
      "usr": 10651399730831737929,
      "detailed_name": "Derived2",
      "short_name": "Derived2",
      "kind": 5,
      "declarations": ["13:56-13:64|-1|1|4"],
      "spell": "11:7-11:15|-1|1|2",
      "extent": "11:1-11:29|-1|1|0",
      "bases": [1],
      "derived": [6],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:56-13:64|-1|1|4"]
    }, {
      "id": 5,
      "usr": 780719166805015998,
      "detailed_name": "T",
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "10:19-10:20|-1|1|2",
      "extent": "10:10-10:20|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["11:24-11:25|-1|1|4"]
    }, {
      "id": 6,
      "usr": 10963370434658308541,
      "detailed_name": "Derived",
      "short_name": "Derived",
      "kind": 5,
      "declarations": ["13:33-13:40|-1|1|4", "13:65-13:72|-1|1|4"],
      "spell": "13:7-13:14|-1|1|2",
      "extent": "13:1-13:76|-1|1|0",
      "bases": [0, 1, 2, 4],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["13:33-13:40|-1|1|4", "13:65-13:72|-1|1|4"]
    }],
  "funcs": [],
  "vars": [{
      "id": 0,
      "usr": 3880651725784125791,
      "detailed_name": "unsigned int T",
      "short_name": "T",
      "declarations": [],
      "spell": "7:23-7:24|-1|1|2",
      "extent": "7:10-7:24|-1|1|0",
      "type": 3,
      "uses": ["8:24-8:25|-1|1|4"],
      "kind": 26,
      "storage": 0
    }]
}
*/
