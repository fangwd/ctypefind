#pragma once

#include <string>
#include <vector>

#include "db.h"

class Indexer {
  private:
    db::Database& db_;

  public:
    Indexer(db::Database& db) : db_(db) {}

    db::Database& db() {
        return db_;
    }

    bool accept(const char* filename);
    bool run(std::vector<std::string>& options);
};
