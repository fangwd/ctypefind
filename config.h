#pragma once

#include <string>
#include <vector>

struct Config {
    std::string db_name;
    bool truncate;
    std::vector<std::string> accept_paths;
    bool verbose;

    Config() : db_name("ctypefind.db"), truncate(false), verbose(false) {
    }
};

extern Config config;
