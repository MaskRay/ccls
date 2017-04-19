struct Parent {
  virtual void Method() = 0;
};
struct Derived : public Parent {
  void Method() override {}
};


void Caller() {
  Derived d;
  Parent* p = &d;

  p->Method();
  d->Method();
}


