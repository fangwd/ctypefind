#include <vector>

struct struct1 {};
class class1 {};

struct T {};
class T2 {
    T* indices_[100];
    T* at(int i) { return indices_[i]; }

    /// Returns the i-th sub-index (const version)
    const T* at(int i) const { return indices_[i]; }
};


}  // namespace ns1
