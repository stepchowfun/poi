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
    std::size_t insert(const std::string &s);
    std::string find(std::size_t id) const;

  private:
    std::unordered_map<std::string, std::size_t> forward_pool;
    std::unordered_map<std::size_t, std::string> reverse_pool;
    std::size_t counter;
  };
}

#endif
