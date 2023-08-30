#pragma once

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <string>
#include <vector>

struct Config {
   private:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    lua_State* L;

   public:
    std::string db_name;
    bool remove_db;

    Config() : L(nullptr), db_name("typefind.db"), remove_db(false) {}

    bool accept(const char* filename);
    bool load(const char* filename);

    ~Config();
};

extern Config config;
