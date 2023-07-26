#pragma once

#include "index_config.h"

namespace veclens {

class IndexManager {
   private:
    faiss::Index* create_faiss_index(const IndexConfig& config);

   public:
};

}  // namespace veclens