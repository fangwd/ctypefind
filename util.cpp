#include "util.h"

#ifdef _WIN32
#include <Windows.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

#include <unistd.h>

#ifdef WIN32
#include <io.h>
#define F_OK 0
#define access _access
#endif

bool file_exists(const char* filename) { return access(filename, F_OK) == 0; }

bool file_exists(const std::string& filename) { return file_exists(filename.c_str()); }

std::string get_exec_name() {
    std::string str;

#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    str = buffer;
#elif __APPLE__
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    _NSGetExecutablePath(buffer, &size);
    str = buffer;
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        str = buffer;
    }
#endif

    return str;
}

std::string get_exec_path() {
    std::string str = get_exec_name();
    for (int i = str.length() - 1; i >= 0; --i) {
        char ch = str[i];
        if (ch == '/' || ch == '\\') {
            str.resize(i);
            break;
        }
    }
    return str;
}
