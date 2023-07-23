#pragma once

#include <string>

struct Config {
    std::string db;
    Config() : db("typefind.db") {}
};

extern Config config;
