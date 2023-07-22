
namespace ns {
template <class T, class U>
class T1 {
    int a;
    T b;
    U c;
};
class T2 {
    T1<float, int> t1;
};
}  // namespace ns
