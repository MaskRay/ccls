struct Bar {};

class Foobar {
 public:
  Foobar() {}
  Foobar(int) {}
  Foobar(int&&, Bar*, bool*) {}
  Foobar(int, Bar*, bool*) {}
};

