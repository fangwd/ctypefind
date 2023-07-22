#include <string>
#include <vector>

typedef std::vector<std::string> StringArray;

#define STRING_ARRAY StringArray

struct T1 {
    StringArray array;
    STRING_ARRAY array2;
};

namespace ns1 {

typedef std::vector<std::string> StringArray;

#define STRING_ARRAY StringArray

struct T1 {
    StringArray array;
    STRING_ARRAY array2;
};

void f(STRING_ARRAY &a, const StringArray*& b,  volatile std::vector<int> *x, int y);

}  // namespace ns1