#include "util.h"

#include <unistd.h>

#ifdef WIN32
#include <io.h>
#define F_OK 0
#define access _access
#endif

bool file_exists(const char *filename) {
return access(filename, F_OK) == 0;
}

bool file_exists(const std::string& filename) {
  return file_exists(filename.c_str());
}