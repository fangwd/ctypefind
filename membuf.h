#ifndef _MEMBUF_H_
#define _MEMBUF_H_

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <streambuf>
#include <string>

struct SimpleBuffer {
    const char *data;
    size_t size;

    SimpleBuffer(const char *s, size_t len) : data(s), size(len) {
    }

    SimpleBuffer(const std::string &s) : SimpleBuffer(s.c_str(), s.size()) {
    }
};

class MemBuf: public std::ostream {
    class MyBuf: public std::streambuf {
      private:
        char *base;
        size_t cap;

      public:
        MyBuf(): base(0), cap(0) {
            setp(base, base + cap);
        }

        ~MyBuf() {
            if (base) free(base);
        }

        size_t size() {
            return pptr() - pbase();
        }

        void resize(size_t new_size) {
            size_t old_size = size();

            if (new_size < old_size) {
                memset(base + new_size, 0, old_size - new_size);
            } else if (new_size > cap) {
                reserve(new_size);
            }
            setp(base, base + cap);
            pbump(new_size);
        }

        void clear() {
            memset(base, 0, cap);
            setp(base, base + cap);
        }

        virtual int_type overflow (int_type c) {
            if (c != EOF) {
                reserve(cap + 1);
                *pptr() = c;
                pbump(1);
            }
            return c;
        }

        virtual std::streamsize xsputn(const char *s, std::streamsize n) {
            return (std::streamsize) append(s, (size_t) n);
        }

        int reserve(size_t new_cap) {
            static const size_t MIN_SIZE = 512;
            static const size_t MAX_SIZE = 268435456;

            if (cap < new_cap) {
                new_cap += MIN_SIZE - 1;
                new_cap /= MIN_SIZE;
                new_cap *= MIN_SIZE;
                if (new_cap <= MAX_SIZE) {
                    char *new_base = (char *) realloc(base, new_cap);
                    if (new_base != NULL) {
                        int size = pptr() - pbase();
                        base = new_base;
                        cap = new_cap;
                        memset(base + size, '\0', cap - size);
                        setp(base, base + cap);
                        pbump(size);
                    } else {
                        return -1;
                    }
                } else {
                    return -2;
                }
            }

            return 0;
        }

        size_t append(const char *s, size_t len) {
            size_t size = this->size();
            if (size + len + 1 > cap) {
                if (reserve(size + len + 1) != 0) {
                    return (size_t) -1;
                }
            }
            memcpy(base + size, s, len);
            pbump((int) len);
            return len;
        }

        void consume(size_t n) {
            assert(n <= size());
            if (n > 0) {
                memmove(base, base + n, size() - n);
                resize(size() - n);
            }
        }

        friend class MemBuf;
    };

    MyBuf *mb;

  public:
    MemBuf() : std::ostream(new MyBuf()) {
        mb = (MyBuf *) rdbuf();
    }

    ~MemBuf() {
        delete mb;
    }

    MemBuf(const MemBuf &other) : std::ostream(new MyBuf()) {
        mb = (MyBuf *) rdbuf();
        append(other.content(), other.size());
    }

    const char *content() const {
        return mb->base;
    }

    const char *end() const {
        return mb->base + mb->size();
    }

    size_t size() const {
        return mb->size();
    }

    void resize(size_t new_size) {
        mb->resize(new_size);
    }

    MemBuf& clear() {
        mb->clear();
        return *this;
    }

    void reset() {
        clear();
    }

    MemBuf &append(const char *s, size_t n=0) {
        if (n == 0) {
            n = strlen(s);
        }
        mb->append(s, n);
        return *this;
    }

    MemBuf &append(int ch) {
        char c = (char) ch;
        return append(&c, 1);
    }

    MemBuf &append(const std::string& s) {
        return append(s.c_str(), s.size());
    }

    MemBuf &append(const SimpleBuffer& s) {
        return append(s.data, s.size);
    }

    int printf(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        int n = vprintf(fmt, args);
        va_end(args);
        return n;
    }

    int vprintf(const char *fmt, va_list ap) {
        size_t size = mb->size();

        while (1) {
            va_list cp;
            int m = mb->cap - size;
            va_copy(cp, ap);
            int n = vsnprintf(mb->base + size, m, fmt, cp);
            va_end(cp);
            if (n > -1 && n < m) {
                mb->pbump(n);
                return n;
            }
            m = n > -1 ? n + 1 : 2 * m;
            if (size + m + 1 > mb->cap) {
                if (mb->reserve(size + m + 1) != 0) {
                    return -1;
                }
            }
        }

        assert(false);

        return -1;
    }

    bool load(const char *filename) {
        FILE  *fp;
        if ((fp = fopen(filename, "rb")) != NULL) {
            fseek(fp, 0, SEEK_END);
            size_t size = (size_t) ftell(fp);
            fseek(fp, 0, SEEK_SET);
            mb->clear();
            bool result = read(fp, size);
            fclose(fp);
            return result;
        }
        return false;
    }

    bool read(FILE *fp, size_t n) {
        mb->reserve(mb->size() + n);
        if (fread(mb->base + mb->size(), 1, n, fp) == n) {
            mb->resize(mb->size() + n);
            return true;
        }
        return false;
    }

    bool save(const char *filename) {
        FILE *fp;
        if ((fp = fopen(filename, "wb")) != NULL) {
            if (fwrite(mb->base, 1, mb->size(), fp) != mb->size()) {
                fclose(fp);
                return false;
            }
            fclose(fp);
            return true;
        }
        return false;
    }

    operator const char *() {
        return content();
    }

    bool operator==(const MemBuf &other) const {
        if (size() != other.size()) {
            return false;
        }
        return !memcmp(content(), other.content(), size());
    }

    bool operator==(const char *s) const {
        if (s && mb->size() == strlen(s))
            return !memcmp(mb->base, s, mb->size());
        else
            return false;
    }

    MemBuf& operator=(const MemBuf &other) {
        clear();
        append(other.content(), other.size());
        return *this;
    }

    char& operator[](const int n) {
        return mb->base[n];
    }

    char last_char() {
        return size() ? mb->base[size() - 1] : '\0';
    }

    void consume(size_t n) {
        return mb->consume(n);
    }

    void truncate(size_t n) {
        assert(n <= mb->size());
        mb->resize(mb->size() - n);
    }

    void *space() {
        return mb->base + mb->size();
    }

    size_t space_size() {
        return mb->cap - mb->size();
    }

    bool reserve(size_t n) {
        if (space_size() < n) {
            return mb->reserve(mb->size() + n) ? false : true;
        }
        return true;
    }

    void appended(size_t n) {
        mb->pbump((int) n);
    }

    bool ends_with(const char *s, size_t len = 0) {
        if (len == 0) len = strlen(s);
        if (mb->size() < len) return false;
        return !memcmp(mb->base + mb->size() - len, s, len);
    }
};

inline std::ostream& operator<<(std::ostream& os, const MemBuf& mb) {
    os.write(mb.content(), mb.size());
    return os;
}

#endif /* _MEMBUF_H_ */
