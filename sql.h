#pragma once

#include "membuf.h"

namespace sql {
class str : public SimpleBuffer {
    bool use_null_for_empty;

   public:
    using SimpleBuffer::SimpleBuffer;

    str(const std::string &s, bool use_null_for_empty = true)
        : SimpleBuffer(s), use_null_for_empty(use_null_for_empty) {}

    friend std::ostream &operator<<(std::ostream &os, const str &s);
};

class id : public SimpleBuffer {
   public:
    using SimpleBuffer::SimpleBuffer;
    friend std::ostream &operator<<(std::ostream &os, const id &s);
};

std::ostream &operator<<(std::ostream &os, const str &s) {
    if (s.size == 0 && s.use_null_for_empty) {
        os << "null";
        return os;
    }

    const char *beg = s.data;
    const char *end = s.data + s.size;

    os << '\'';

    while (beg < end) {
        char c = *beg;
        if (c == '\'') {
            os << c;
        }
        os << c;
        beg++;
    }

    os << '\'';

    return os;
}

std::ostream &operator<<(std::ostream &os, const id &s) {
    const char *beg = s.data;
    const char *end = s.data + s.size;

    os << '`';

    while (beg < end) {
        char c = *beg;
        if (c == '\\' || c == '`') {
            os << '\\';
        }
        os << c;
        beg++;
    }

    os << '`';

    return os;
}

struct field {
    enum class Type {
        Bool,
        String,
        Integer,
    };

    Type type;
    const std::string &name;
    union {
        const std::string *s;
        int n;
        bool b;
    } value;
    bool is_first;
    bool use_null_for_empty;

    field(const std::string &name, const std::string &value, bool is_first = false, bool use_null_for_empty = true)
        : type(Type::String),
          name(name),
          value{.s = &value},
          is_first(is_first),
          use_null_for_empty(use_null_for_empty) {}

    field(const std::string &name, int value, bool is_first = false)
        : type(Type::Integer), name(name), value{.n = value}, is_first(is_first) {}

    field(const std::string &name, bool value, bool is_first = false)
        : type(Type::Bool), name(name), value{.b = value}, is_first(is_first) {}
};

std::ostream &operator<<(std::ostream &os, const field &f) {
    if (!f.is_first) {
        os << ", ";
    }

    os << id(f.name) << " = ";

    switch (f.type) {
        case field::Type::String:
            os << str(*f.value.s, f.use_null_for_empty);
            break;
        case field::Type::Integer:
            os << f.value.n;
            break;
        case field::Type::Bool:
            os << (f.value.b ? "true" : "false");
            break;
    }
    return os;
}

struct pk {
    int value;
    pk(int value) : value(value) {}
};

std::ostream &operator<<(std::ostream &os, const pk &pk) {
    if (pk.value <= 0) {
        os << "null";
    } else {
        os << pk.value;
    }
    return os;
}

}  // namespace sql