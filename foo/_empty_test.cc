#if false
#include <string>
#include <vector>

struct MyBar {
  int x;
  int aaaa1;
  int aaaa2;
  int aaaa3;
  static int foobez;

  // This is some awesome docs.
  float MemberFunc(int a, char b, std::vector<int> foo);
  float MemberFunc2(int a, char b, std::vector<int> foo);

  // The names are some extra state.
  std::vector<std::string> names;
};

int MyBar::foobez;

int foo() {
  int a = 10;
  MyBar foooo;
  MyBar f;
  MyBar f2;
}

float MyBar::MemberFunc(int a, char b, std::vector<int> foo) {
  this->x = 100;
  this->MemberFunc(0, 0, {});

  return ::foo();
}
#endif