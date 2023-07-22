class Class1 {};

struct Struct1 {};

class AbstractClass1 {
    virtual int f() = 0;
    virtual int g() = 0;
};

class AbstractClass2 : public AbstractClass1 {
    int g() override { return 1; }
};

class Class2 : public AbstractClass2 {
    int f() override { return 0; }
};

template <class T>
class Template1 {
    T t;
};