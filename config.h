#pragma once

#include <string>

struct Config {
    std::string db_name;
    bool remove_db;
    Config() : db_name("typefind.db"), remove_db(false) {}
};

extern Config config;
