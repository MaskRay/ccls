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
      "qualified_name": "Base1",
      "definition": "1:2:7",
      "derived": [2, 5],
      "uses": ["*1:2:7", "*1:8:18", "*1:13:17"]
    }, {
      "id": 1,
      "usr": "c:@ST>1#T@Base2",
      "short_name": "Base2",
      "qualified_name": "Base2",
      "definition": "1:5:7",
      "derived": [3, 5],
      "uses": ["*1:5:7", "*1:11:18", "*1:13:27"]
    }, {
      "id": 2,
      "usr": "c:@ST>1#Ni@Derived1",
      "short_name": "Derived1",
      "qualified_name": "Derived1",
      "definition": "1:8:7",
      "parents": [0],
      "derived": [5],
      "uses": ["*1:8:7", "*1:13:43"]
    }, {
      "id": 3,
      "usr": "c:@ST>1#T@Derived2",
      "short_name": "Derived2",
      "qualified_name": "Derived2",
      "definition": "1:11:7",
      "parents": [1],
      "derived": [5],
      "uses": ["*1:11:7", "*1:13:56"]
    }, {
      "id": 5,
      "usr": "c:@S@Derived",
      "short_name": "Derived",
      "qualified_name": "Derived",
      "definition": "1:13:7",
      "parents": [0, 1, 2, 3],
      "uses": ["*1:13:7", "*1:13:33", "*1:13:65"]
    }],
  "functions": [],
  "variables": []
}
*/