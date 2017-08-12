/*
  This header declares some utility functions.
*/

#ifndef POI_UTIL_H
#define POI_UTIL_H

#include <memory>
#include <poi/ast.h>
#include <poi/string_pool.h>
#include <unordered_set>

namespace Poi {
  void variables_from_pattern(
    std::unordered_set<size_t> &variables,
    std::shared_ptr<Poi::Pattern> pattern,
    Poi::StringPool &pool
  );
}

#endif
