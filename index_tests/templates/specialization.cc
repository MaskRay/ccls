template<class T>
class function;

template<typename T, typename... Args>
class function<T(Args...)> {};

function<void(int)> f;

template<typename T> class allocator;

template<typename T, typename Alloc = allocator<T> >
class vector {
  void clear();
};

template<typename T>
class vector<T*> {};

struct Z1 {};

template class vector<Z1>;

struct Z2 {};

template<>
class vector<Z2> {
  void clear();
};

vector<char> vc;
vector<int*> vip;
vector<Z1> vz1;
vector<Z2> vz2;

enum Enum {
  Enum0, Enum1
};
template <typename T, int I, Enum, int E>
void foo(T Value) {}

static const int kOnst = 7;
template <>
void foo<float, 9, Enum0, kOnst + 7>(float Value);

/*
OUTPUT:
{
  "includes": [],
  "skipped_by_preprocessor": [],
  "types": [{
      "id": 0,
      "usr": 15019211479263750068,
      "detailed_name": "function",
      "short_name": "function",
      "kind": 5,
      "declarations": ["2:7-2:15|-1|1|1", "5:7-5:15|-1|1|4"],
      "spell": "2:7-2:15|-1|1|2",
      "extent": "1:1-2:15|-1|1|0",
      "bases": [],
      "derived": [1],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["7:1-7:9|-1|1|4"]
    }, {
      "id": 1,
      "usr": 218068462278884837,
      "detailed_name": "function",
      "short_name": "function",
      "kind": 5,
      "declarations": [],
      "spell": "5:7-5:15|-1|1|2",
      "extent": "4:1-5:30|-1|1|0",
      "bases": [0],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [0],
      "uses": ["7:1-7:9|-1|1|4"]
    }, {
      "id": 2,
      "usr": 10862637711685426953,
      "detailed_name": "T",
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "4:19-4:20|-1|1|2",
      "extent": "4:10-4:20|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["5:16-5:17|-1|1|4"]
    }, {
      "id": 3,
      "usr": 756188769017350739,
      "detailed_name": "Args",
      "short_name": "Args",
      "kind": 26,
      "declarations": [],
      "spell": "4:34-4:38|-1|1|2",
      "extent": "4:22-4:38|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["5:18-5:22|-1|1|4"]
    }, {
      "id": 4,
      "usr": 15695704394170757108,
      "detailed_name": "allocator",
      "short_name": "allocator",
      "kind": 5,
      "declarations": ["9:28-9:37|-1|1|1", "11:39-11:48|-1|1|4"],
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "id": 5,
      "usr": 7440942986741176606,
      "detailed_name": "vector",
      "short_name": "vector",
      "kind": 5,
      "declarations": ["17:7-17:13|-1|1|4", "26:7-26:13|-1|1|4"],
      "spell": "12:7-12:13|-1|1|2",
      "extent": "12:1-14:2|-1|1|0",
      "bases": [],
      "derived": [6, 10],
      "types": [],
      "funcs": [0],
      "vars": [],
      "instances": [1, 3, 4],
      "uses": ["30:1-30:7|-1|1|4", "31:1-31:7|-1|1|4", "32:1-32:7|-1|1|4", "33:1-33:7|-1|1|4"]
    }, {
      "id": 6,
      "usr": 16155717907537731864,
      "detailed_name": "vector",
      "short_name": "vector",
      "kind": 5,
      "declarations": [],
      "spell": "17:7-17:13|-1|1|2",
      "extent": "16:1-17:20|-1|1|0",
      "bases": [5],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [2],
      "uses": ["31:1-31:7|-1|1|4"]
    }, {
      "id": 7,
      "usr": 3421332160420436276,
      "detailed_name": "T",
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "16:19-16:20|-1|1|2",
      "extent": "16:10-16:20|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["17:14-17:15|-1|1|4"]
    }, {
      "id": 8,
      "usr": 5760043510674081814,
      "detailed_name": "Z1",
      "short_name": "Z1",
      "kind": 23,
      "declarations": [],
      "spell": "19:8-19:10|-1|1|2",
      "extent": "19:1-19:13|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["32:8-32:10|-1|1|4"]
    }, {
      "id": 9,
      "usr": 10124869160135436852,
      "detailed_name": "Z2",
      "short_name": "Z2",
      "kind": 23,
      "declarations": ["26:14-26:16|-1|1|4"],
      "spell": "23:8-23:10|-1|1|2",
      "extent": "23:1-23:13|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["33:8-33:10|-1|1|4"]
    }, {
      "id": 10,
      "usr": 1663022413889915338,
      "detailed_name": "vector",
      "short_name": "vector",
      "kind": 5,
      "declarations": [],
      "spell": "26:7-26:13|-1|1|2",
      "extent": "25:1-28:2|-1|1|0",
      "bases": [5],
      "derived": [],
      "types": [],
      "funcs": [1],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "id": 11,
      "usr": 9201299975592934124,
      "detailed_name": "Enum",
      "short_name": "Enum",
      "kind": 10,
      "declarations": [],
      "spell": "35:6-35:10|-1|1|2",
      "extent": "35:1-37:2|-1|1|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "id": 12,
      "usr": 2461355892344618654,
      "detailed_name": "T",
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "38:20-38:21|2|3|2",
      "extent": "38:11-38:21|2|3|0",
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["39:10-39:11|-1|1|4"]
    }, {
      "id": 13,
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
      "instances": [8],
      "uses": []
    }],
  "funcs": [{
      "id": 0,
      "usr": 18107614608385228556,
      "detailed_name": "void vector::clear()",
      "short_name": "clear",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "13:8-13:13|5|2|1",
          "param_spellings": []
        }],
      "declaring_type": 5,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 1,
      "usr": 6113470698424012876,
      "detailed_name": "void vector::clear()",
      "short_name": "clear",
      "kind": 6,
      "storage": 1,
      "declarations": [{
          "spell": "27:8-27:13|10|2|1",
          "param_spellings": []
        }],
      "declaring_type": 10,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "id": 2,
      "usr": 17498190318698490707,
      "detailed_name": "void foo(T Value)",
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": [{
          "spell": "43:6-43:9|-1|1|1",
          "param_spellings": ["43:44-43:49"]
        }],
      "spell": "39:6-39:9|-1|1|2",
      "extent": "39:1-39:21|-1|1|0",
      "bases": [],
      "derived": [],
      "vars": [7],
      "uses": [],
      "callees": []
    }],
  "vars": [{
      "id": 0,
      "usr": 2933643612409209903,
      "detailed_name": "function<void (int)> f",
      "short_name": "f",
      "declarations": [],
      "spell": "7:21-7:22|-1|1|2",
      "extent": "7:1-7:22|-1|1|0",
      "type": 1,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 1,
      "usr": 5792869548777559988,
      "detailed_name": "vector<char> vc",
      "short_name": "vc",
      "declarations": [],
      "spell": "30:14-30:16|-1|1|2",
      "extent": "30:1-30:16|-1|1|0",
      "type": 5,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 2,
      "usr": 86949563628772958,
      "detailed_name": "vector<int *> vip",
      "short_name": "vip",
      "declarations": [],
      "spell": "31:14-31:17|-1|1|2",
      "extent": "31:1-31:17|-1|1|0",
      "type": 6,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 3,
      "usr": 3566687051827176322,
      "detailed_name": "vector<Z1> vz1",
      "short_name": "vz1",
      "declarations": [],
      "spell": "32:12-32:15|-1|1|2",
      "extent": "32:1-32:15|-1|1|0",
      "type": 5,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 4,
      "usr": 15931696253641284761,
      "detailed_name": "vector<Z2> vz2",
      "short_name": "vz2",
      "declarations": [],
      "spell": "33:12-33:15|-1|1|2",
      "extent": "33:1-33:15|-1|1|0",
      "type": 5,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "id": 5,
      "usr": 15477793821005285152,
      "detailed_name": "Enum::Enum0",
      "short_name": "Enum0",
      "hover": "Enum::Enum0 = 0",
      "declarations": [],
      "spell": "36:3-36:8|11|2|2",
      "extent": "36:3-36:8|11|2|0",
      "type": 11,
      "uses": ["43:20-43:25|-1|1|4"],
      "kind": 22,
      "storage": 0
    }, {
      "id": 6,
      "usr": 4917621020431490070,
      "detailed_name": "Enum::Enum1",
      "short_name": "Enum1",
      "hover": "Enum::Enum1 = 1",
      "declarations": [],
      "spell": "36:10-36:15|11|2|2",
      "extent": "36:10-36:15|11|2|0",
      "type": 11,
      "uses": [],
      "kind": 22,
      "storage": 0
    }, {
      "id": 7,
      "usr": 10307767688451422448,
      "detailed_name": "T Value",
      "short_name": "Value",
      "declarations": [],
      "spell": "39:12-39:17|2|3|2",
      "extent": "39:10-39:17|2|3|0",
      "uses": [],
      "kind": 253,
      "storage": 1
    }, {
      "id": 8,
      "usr": 13914496963221806870,
      "detailed_name": "const int kOnst",
      "short_name": "kOnst",
      "hover": "const int kOnst = 7",
      "declarations": [],
      "spell": "41:18-41:23|-1|1|2",
      "extent": "41:1-41:27|-1|1|0",
      "type": 13,
      "uses": ["43:27-43:32|-1|1|4"],
      "kind": 13,
      "storage": 3
    }]
}
*/
