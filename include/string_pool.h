/*
  This header declares the interface to the string pool.
*/

#ifndef POI_STRING_POOL_H
#define POI_STRING_POOL_H

#include <string>
#include <unordered_map>

namespace Poi {
  // A StringPool assigns an ID to every string. Two strings will have the
  // same ID if and only if they are equal.
  class StringPool {
  public:
    explicit StringPool();
    size_t insert(const std::string &s);
    std::string find(size_t id) const;

  private:
    std::unordered_map<std::string, size_t> forward_pool;
    std::unordered_map<size_t, std::string> reverse_pool;
    size_t counter;
  };
}

#endif
