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
                config.db= argv[++i];
            } else {
                std::cerr << "Unknown option: '" << arg << "'\n";
                return 1;
            }
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    std::vector<std::string> options;
    int options_error = parse_options(argc, argv, options);
    if (options_error) {
        return options_error;
    }

    db::Database db(config.db);

    if (int error = db.clear()) {
        std::cerr << "Failed to clear the database: " << error << "\n";
        return 1;
    }

    Indexer indexer(db);

    indexer.run(options);

    return 0;
}
