namespace ns1 {
typedef long long idx_t;

using score_t = double;

class T1 {};

class T2 {
    int f(idx_t x, score_t y, T1& z, int* d) { return 0; }
};

template <class X>
int f(const X& x);

template <class Y, class Z>
class T3 {
    Y y;
    Z z;
};

// type: { decl_name: ns1::T2, name: ns1::T2<double,float>, decl_type: class}
int g(T3<double, float>& z) { return 1; }

}  // namespace ns1
