#include <iostream>

#include "indexer.h"
#include "util.h"

int main(int argc, char **argv) {
    if (file_exists("DEBUG") && file_exists("faiss.db")) {
        if (remove("faiss.db") != 0) {
            std::cerr << "Failed to remove faiss.db, error: " << errno << '\n';
        }
    }

    db::Database db("faiss.db");

    if (int error = db.clear()) {
        std::cerr << "Failed to clear the database: " << error << "\n";
        return 1;
    }

    std::vector<std::string> options;

    for (int i = 1; i < argc; i++) {
        options.push_back(argv[i]);
    }

    Indexer indexer(db);

    indexer.run(options);

    return 0;
}
