// func1
void func1() {
}

namespace ns {
 class T {

 };
}

#define TT ns::T
namespace ns1 {
  const int func1(TT t);

  int func1(ns::T t, float b) {
    return 0;
  }

  const int func1(ns::T t) {
    return 1;
  }
}