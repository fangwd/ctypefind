// class 1
class class1 {};

struct struct1 {};

union union1 {};

enum enum1 {};

namespace ns {

class class1 {};

class struct1 {};

/**
 * @brief union 1
 * is a union
 */
union union1 {};

// enum 1
enum class enum1 {};

}  // namespace ns

class abstract1 {
    virtual int f() = 0;
    virtual int g() = 0;
};

class abstract2 : public abstract1 {
    int g() override { return 1; }
};

class nonabstract1 : public abstract2 {
    int f() override { return 0; }
};
