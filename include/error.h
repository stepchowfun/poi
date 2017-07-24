/*
  This header declares a class for errors reported by the compiler.
*/

#ifndef POI_ERROR_H
#define POI_ERROR_H

#include <string>

namespace poi {

  class Error {
  private:
    std::string message;

  public:
    explicit Error(std::string message);
    explicit Error(
      std::string message, // No trailing line break
      const std::string &source,
      std::string source_name,
      size_t start_pos, // Inclusive
      size_t end_pos // Exclusive
    );
    std::string what(); // No trailing line break
  };

}

#endif
