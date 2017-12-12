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
  "types": [{
      "id": 0,
      "usr": "c:@ST>1#Ni@Base1",
      "short_name": "Base1",
      "detailed_name": "Base1",
      "definition_spelling": "2:7-2:12",
      "definition_extent": "2:1-2:15",
      "derived": [2, 5],
      "uses": ["2:7-2:12", "8:18-8:23", "13:17-13:22"]
    }, {
      "id": 1,
      "usr": "c:@ST>1#T@Base2",
      "short_name": "Base2",
      "detailed_name": "Base2",
      "definition_spelling": "5:7-5:12",
      "definition_extent": "5:1-5:15",
      "derived": [3, 5],
      "uses": ["5:7-5:12", "11:18-11:23", "13:27-13:32"]
    }, {
      "id": 2,
      "usr": "c:@ST>1#Ni@Derived1",
      "short_name": "Derived1",
      "detailed_name": "Derived1",
      "definition_spelling": "8:7-8:15",
      "definition_extent": "8:1-8:29",
      "parents": [0],
      "derived": [5],
      "uses": ["8:7-8:15", "13:43-13:51"]
    }, {
      "id": 3,
      "usr": "c:@ST>1#T@Derived2",
      "short_name": "Derived2",
      "detailed_name": "Derived2",
      "definition_spelling": "11:7-11:15",
      "definition_extent": "11:1-11:29",
      "parents": [1],
      "derived": [5],
      "uses": ["11:7-11:15", "13:56-13:64"]
    }, {
      "id": 4,
      "usr": "c:class_inherit_templated_parent.cc@154",
      "short_name": "",
      "detailed_name": "",
      "uses": ["11:24-11:25"]
    }, {
      "id": 5,
      "usr": "c:@S@Derived",
      "short_name": "Derived",
      "detailed_name": "Derived",
      "definition_spelling": "13:7-13:14",
      "definition_extent": "13:1-13:76",
      "parents": [0, 1, 2, 3],
      "uses": ["13:7-13:14", "13:33-13:40", "13:65-13:72"]
    }]
}
*/
