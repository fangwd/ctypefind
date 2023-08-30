#include "config.h"

bool Config::accept(const char* filename) {
    if (!L) {
        return true;
    }

    lua_getglobal(L, "accept");

    if (lua_type(L, -1) != LUA_TFUNCTION) {
        fprintf(stderr, "Error: 'accept' is not a function (%s).\n", lua_typename(L, -1));
        lua_pop(L, 1);
        return false;
    }

    if (!filename) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, filename);
    }

    lua_pcall(L, 1, 1, 0);
    bool result = (bool)lua_toboolean(L, -1);
    lua_pop(L, 1);

    return result;
}

bool Config::load(const char* filename) {
    if (L) {
        lua_close(L);
    }

    L = luaL_newstate();

    luaL_openlibs(L);

    if (luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0)) {
        const char* error = lua_tostring(L, -1);
        fprintf(stderr, "Error loading %s:: %s\n", filename, error);
        lua_pop(L, 1);
        lua_close(L);
        L = nullptr;
        return false;
    }

    return true;
}

Config::~Config() {
    if (L) {
        lua_close(L);
    }
}

Config config;
