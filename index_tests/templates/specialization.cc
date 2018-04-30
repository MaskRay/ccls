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
  "usr2func": [{
      "usr": 6113470698424012876,
      "detailed_name": "void vector::clear()",
      "qual_name_offset": 5,
      "short_name": "clear",
      "kind": 6,
      "storage": 1,
      "declarations": ["27:8-27:13|1663022413889915338|2|1"],
      "declaring_type": 1663022413889915338,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
      "callees": []
    }, {
      "usr": 17498190318698490707,
      "detailed_name": "void foo(T Value)",
      "qual_name_offset": 5,
      "short_name": "foo",
      "kind": 12,
      "storage": 1,
      "declarations": ["43:6-43:9|0|1|1"],
      "spell": "39:6-39:9|0|1|2",
      "extent": "39:1-39:21|0|1|0",
      "declaring_type": 0,
      "bases": [],
      "derived": [],
      "vars": [17826688417349629938],
      "uses": [],
      "callees": []
    }, {
      "usr": 18107614608385228556,
      "detailed_name": "void vector::clear()",
      "qual_name_offset": 5,
      "short_name": "clear",
      "kind": 6,
      "storage": 1,
      "declarations": ["13:8-13:13|7440942986741176606|2|1"],
      "declaring_type": 7440942986741176606,
      "bases": [],
      "derived": [],
      "vars": [],
      "uses": [],
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
      "instances": [13914496963221806870],
      "uses": []
    }, {
      "usr": 218068462278884837,
      "detailed_name": "function",
      "qual_name_offset": 0,
      "short_name": "function",
      "kind": 5,
      "declarations": [],
      "spell": "5:7-5:15|0|1|2",
      "extent": "4:1-5:30|0|1|0",
      "alias_of": 0,
      "bases": [15019211479263750068],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [2933643612409209903],
      "uses": ["7:1-7:9|0|1|4"]
    }, {
      "usr": 1663022413889915338,
      "detailed_name": "vector",
      "qual_name_offset": 0,
      "short_name": "vector",
      "kind": 5,
      "declarations": [],
      "spell": "26:7-26:13|0|1|2",
      "extent": "25:1-28:2|0|1|0",
      "alias_of": 0,
      "bases": [7440942986741176606],
      "derived": [],
      "types": [],
      "funcs": [6113470698424012876],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 5760043510674081814,
      "detailed_name": "Z1",
      "qual_name_offset": 0,
      "short_name": "Z1",
      "kind": 23,
      "declarations": [],
      "spell": "19:8-19:10|0|1|2",
      "extent": "19:1-19:13|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["32:8-32:10|0|1|4"]
    }, {
      "usr": 7143192229126273961,
      "detailed_name": "Args",
      "qual_name_offset": 0,
      "short_name": "Args",
      "kind": 26,
      "declarations": [],
      "spell": "4:34-4:38|0|1|2",
      "extent": "4:22-4:38|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["5:18-5:22|0|1|4"]
    }, {
      "usr": 7440942986741176606,
      "detailed_name": "vector",
      "qual_name_offset": 0,
      "short_name": "vector",
      "kind": 5,
      "declarations": ["17:7-17:13|0|1|4", "26:7-26:13|0|1|4"],
      "spell": "12:7-12:13|0|1|2",
      "extent": "12:1-14:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [16155717907537731864, 1663022413889915338],
      "types": [],
      "funcs": [18107614608385228556],
      "vars": [],
      "instances": [5792869548777559988, 3566687051827176322, 15931696253641284761],
      "uses": ["30:1-30:7|0|1|4", "31:1-31:7|0|1|4", "32:1-32:7|0|1|4", "33:1-33:7|0|1|4"]
    }, {
      "usr": 8880262253425334092,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "16:19-16:20|0|1|2",
      "extent": "16:10-16:20|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["17:14-17:15|0|1|4"]
    }, {
      "usr": 9201299975592934124,
      "detailed_name": "Enum",
      "qual_name_offset": 0,
      "short_name": "Enum",
      "kind": 10,
      "declarations": [],
      "spell": "35:6-35:10|0|1|2",
      "extent": "35:1-37:2|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 9673599782548740467,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "4:19-4:20|0|1|2",
      "extent": "4:10-4:20|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["5:16-5:17|0|1|4"]
    }, {
      "usr": 10124869160135436852,
      "detailed_name": "Z2",
      "qual_name_offset": 0,
      "short_name": "Z2",
      "kind": 23,
      "declarations": ["26:14-26:16|0|1|4"],
      "spell": "23:8-23:10|0|1|2",
      "extent": "23:1-23:13|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["33:8-33:10|0|1|4"]
    }, {
      "usr": 14111105212951082474,
      "detailed_name": "T",
      "qual_name_offset": 0,
      "short_name": "T",
      "kind": 26,
      "declarations": [],
      "spell": "38:20-38:21|17498190318698490707|3|2",
      "extent": "38:11-38:21|17498190318698490707|3|0",
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["39:10-39:11|0|1|4"]
    }, {
      "usr": 15019211479263750068,
      "detailed_name": "function",
      "qual_name_offset": 0,
      "short_name": "function",
      "kind": 5,
      "declarations": ["2:7-2:15|0|1|1", "5:7-5:15|0|1|4"],
      "spell": "2:7-2:15|0|1|2",
      "extent": "1:1-2:15|0|1|0",
      "alias_of": 0,
      "bases": [],
      "derived": [218068462278884837],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": ["7:1-7:9|0|1|4"]
    }, {
      "usr": 15695704394170757108,
      "detailed_name": "allocator",
      "qual_name_offset": 0,
      "short_name": "allocator",
      "kind": 5,
      "declarations": ["9:28-9:37|0|1|1", "11:39-11:48|0|1|4"],
      "alias_of": 0,
      "bases": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [],
      "uses": []
    }, {
      "usr": 16155717907537731864,
      "detailed_name": "vector",
      "qual_name_offset": 0,
      "short_name": "vector",
      "kind": 5,
      "declarations": [],
      "spell": "17:7-17:13|0|1|2",
      "extent": "16:1-17:20|0|1|0",
      "alias_of": 0,
      "bases": [7440942986741176606],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [86949563628772958],
      "uses": ["31:1-31:7|0|1|4"]
    }],
  "usr2var": [{
      "usr": 86949563628772958,
      "detailed_name": "vector<int *> vip",
      "qual_name_offset": 14,
      "short_name": "vip",
      "declarations": [],
      "spell": "31:14-31:17|0|1|2",
      "extent": "31:1-31:17|0|1|0",
      "type": 16155717907537731864,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 2933643612409209903,
      "detailed_name": "function<void (int)> f",
      "qual_name_offset": 21,
      "short_name": "f",
      "declarations": [],
      "spell": "7:21-7:22|0|1|2",
      "extent": "7:1-7:22|0|1|0",
      "type": 218068462278884837,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 3566687051827176322,
      "detailed_name": "vector<Z1> vz1",
      "qual_name_offset": 11,
      "short_name": "vz1",
      "declarations": [],
      "spell": "32:12-32:15|0|1|2",
      "extent": "32:1-32:15|0|1|0",
      "type": 7440942986741176606,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 4917621020431490070,
      "detailed_name": "Enum::Enum1",
      "qual_name_offset": 0,
      "short_name": "Enum1",
      "hover": "Enum::Enum1 = 1",
      "declarations": [],
      "spell": "36:10-36:15|9201299975592934124|2|2",
      "extent": "36:10-36:15|9201299975592934124|2|0",
      "type": 9201299975592934124,
      "uses": [],
      "kind": 22,
      "storage": 0
    }, {
      "usr": 5792869548777559988,
      "detailed_name": "vector<char> vc",
      "qual_name_offset": 13,
      "short_name": "vc",
      "declarations": [],
      "spell": "30:14-30:16|0|1|2",
      "extent": "30:1-30:16|0|1|0",
      "type": 7440942986741176606,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 13914496963221806870,
      "detailed_name": "const int kOnst",
      "qual_name_offset": 10,
      "short_name": "kOnst",
      "hover": "const int kOnst = 7",
      "declarations": [],
      "spell": "41:18-41:23|0|1|2",
      "extent": "41:1-41:27|0|1|0",
      "type": 17,
      "uses": ["43:27-43:32|0|1|4"],
      "kind": 13,
      "storage": 3
    }, {
      "usr": 15477793821005285152,
      "detailed_name": "Enum::Enum0",
      "qual_name_offset": 0,
      "short_name": "Enum0",
      "hover": "Enum::Enum0 = 0",
      "declarations": [],
      "spell": "36:3-36:8|9201299975592934124|2|2",
      "extent": "36:3-36:8|9201299975592934124|2|0",
      "type": 9201299975592934124,
      "uses": ["43:20-43:25|0|1|4"],
      "kind": 22,
      "storage": 0
    }, {
      "usr": 15931696253641284761,
      "detailed_name": "vector<Z2> vz2",
      "qual_name_offset": 11,
      "short_name": "vz2",
      "declarations": [],
      "spell": "33:12-33:15|0|1|2",
      "extent": "33:1-33:15|0|1|0",
      "type": 7440942986741176606,
      "uses": [],
      "kind": 13,
      "storage": 1
    }, {
      "usr": 17826688417349629938,
      "detailed_name": "T Value",
      "qual_name_offset": 2,
      "short_name": "Value",
      "declarations": [],
      "spell": "39:12-39:17|17498190318698490707|3|2",
      "extent": "39:10-39:17|17498190318698490707|3|0",
      "type": 0,
      "uses": [],
      "kind": 253,
      "storage": 1
    }]
}
*/
