class Foo {
  void operator()(int) { }
  void operator()(bool);
  int operator()(int a, int b);
};

friend Foo &operator += (const Foo&, const Type&);

/*
OUTPUT:
{
  "types": [{
      "id": 0,
      "usr": "c:@S@Foo",
      "short_name": "Foo",
      "detailed_name": "Foo",
      "definition_spelling": "1:7-1:10",
      "definition_extent": "1:1-5:2",
      "funcs": [0, 1, 2],
      "uses": ["1:7-1:10", "7:8-7:11", "7:32-7:35"]
    }],
  "funcs": [{
      "id": 0,
      "is_operator": true,
      "usr": "c:@S@Foo@F@operator()#I#",
      "short_name": "operator()",
      "detailed_name": "void Foo::operator()(int)",
      "definition_spelling": "2:8-2:18",
      "definition_extent": "2:3-2:27",
      "declaring_type": 0
    }, {
      "id": 1,
      "is_operator": true,
      "usr": "c:@S@Foo@F@operator()#b#",
      "short_name": "operator()",
      "detailed_name": "void Foo::operator()(bool)",
      "declarations": [{
          "spelling": "3:8-3:18",
          "extent": "3:3-3:24",
          "content": "void operator()(bool)",
          "param_spellings": ["3:23-3:23"]
        }],
      "declaring_type": 0
    }, {
      "id": 2,
      "is_operator": true,
      "usr": "c:@S@Foo@F@operator()#I#I#",
      "short_name": "operator()",
      "detailed_name": "int Foo::operator()(int, int)",
      "declarations": [{
          "spelling": "4:7-4:17",
          "extent": "4:3-4:31",
          "content": "int operator()(int a, int b)",
          "param_spellings": ["4:22-4:23", "4:29-4:30"]
        }],
      "declaring_type": 0
    }, {
      "id": 3,
      "is_operator": true,
      "usr": "c:@F@operator+=#&1$@S@Foo#&1I#",
      "short_name": "operator+=",
      "detailed_name": "Foo &operator+=(const Foo &, const int &)",
      "declarations": [{
          "spelling": "7:13-7:24",
          "extent": "7:1-7:50",
          "content": "friend Foo &operator += (const Foo&, const Type&)",
          "param_spellings": ["7:36-7:36", "7:49-7:49"]
        }]
    }]
}
*/
