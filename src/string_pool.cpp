#include <poi/error.h>
#include <poi/string_pool.h>

poi::StringPool::StringPool() {
  counter = 0;
}

size_t poi::StringPool::insert(std::string &s) {
  auto iter = forward_pool.find(s);
  if (iter == forward_pool.end()) {
    auto index = counter;
    ++counter;
    forward_pool.insert({ s, index });
    reverse_pool.insert({ index, s });
    return index;
  } else {
    return iter->second;
  }
}

std::string poi::StringPool::find(size_t id) {
  auto iter = reverse_pool.find(id);
  if (iter == reverse_pool.end()) {
    throw poi::Error(
      "'" + std::to_string(id) + "' is not in the string pool."
    );
  } else {
    return iter->second;
  }
}