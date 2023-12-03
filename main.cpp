#include <iostream>

#include "config.h"
#include "indexer.h"
#include "util.h"

static int parse_options(int argc, char **argv, std::vector<std::string> &compiler_options) {
#define check_arg(arg)                                                    \
    do                                                                    \
        if (i == argc - 1) {                                              \
            std::cerr << "Error: missing argument for '" << arg << "'\n"; \
            return 1;                                                     \
        }                                                                 \
    while (0)

    bool set_compiler_options = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (set_compiler_options) {
            compiler_options.push_back(arg);
        } else if (arg == "--") {
            set_compiler_options = true;
        } else {
            if (arg == "--db") {
                check_arg(arg);
                config.db_name = argv[++i];
            } else if (arg == "--truncate") {
                config.truncate = true;
            } else if (arg == "--accept") {
                check_arg(arg);
                config.accept_paths.push_back(argv[++i]);
            } else if (arg == "--verbose") {
                config.verbose = true;
            } else {
                std::cerr << "Unknown option: '" << arg << "'\n";
                return 1;
            }
        }
    }
    return 0;
}

static void print_usage(const char *app);

int main(int argc, char **argv) {
    std::vector<std::string> options;

    int options_error = parse_options(argc, argv, options);
    if (options_error) {
        return options_error;
    }

    if (options.size() == 0) {
        print_usage(argv[0]);
        return 1;
    }

    db::Database db(config.db_name);

    if (config.truncate && db.clear() != 0) {
        std::cerr << "Failed to truncate '" << config.db_name << "'\n";
        return 1;
    }

    Indexer indexer(db);

    bool success = indexer.run(options);

    return success ? 0 : 1;
}

static void print_usage(const char *app) {
    std::cout << "Usage: " << app << " [OPTIONS] -- <COMPILER OPTIONS>\n";

    std::cout << "\n";
    std::cout << "OPTIONS:\n";
    std::cout << "--db <dbname>\tDatabase name\n";
    std::cout << "--accept <str>\tOnly file names containing <str> will be accepted\n";
    std::cout << "--truncate\tTruncate existing tables";
    std::cout << "--verbose\tPrints file names visited\n";

    std::cout << "\n";
    std::cout << "COMPILER OPTIONS:\tOptions for C++ compiler (clang)\n";

    std::cout << "\n";
    std::cout << "Example:\n";
    std::cout << app << " --db app.db --accept app/ -- -std=c++17 -I/usr/local/include -c app/main.cpp\n";
}