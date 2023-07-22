#include <vector>

using namespace std;

template <class T>
class T3 {
    T t;
};

template <class T = int>
class T4 {
    T t;
};

template <int N, class T>
class MyArray {
   public:
    int get(int index) { return data[index]; }

   private:
    T data[N];
};

template <int N = 100>
class MyArray2 {
   public:
    int get(int index) { return data[index]; }

   private:
    int data[N];
};

template <template <typename> class T>
class MyContainer {
   public:
    void add(typename T<int>::value_type value);
    typename T<int>::value_type get(int index);

   private:
    T<int> data;
};

namespace ns1 {
template <template <typename> class T = vector>
class MyContainer2 {
   public:
    void add(typename T<int>::value_type value);
    typename T<int>::value_type get(int index);

   private:
    T<int> data;
};

template<typename U, typename... Ts>
struct valid;

template<typename...>
struct Tuple {};
 
template<typename T1, typename T2>
struct Pair {};
 
template<class... Args1>
struct zip
{
    template<class... Args2>
    struct with
    {
        typedef Tuple<Pair<Args1, Args2>...> type;
        // Pair<Args1, Args2>... is the pack expansion
        // Pair<Args1, Args2> is the pattern
    };
};
 
}  // namespace ns1