#pragma once

#include <string>
#include <vector>

struct Config {
    std::string db_name;
    bool remove_db;
    std::vector<std::string> accept_paths;

    Config() : db_name("typefind.db"), remove_db(false) {
        accept_paths.push_back(".");
    }
};

extern Config config;
