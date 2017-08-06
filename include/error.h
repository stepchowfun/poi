/*
  This header declares a class for reporting errors with nice formatting.
*/

#ifndef POI_ERROR_H
#define POI_ERROR_H

#include <string>

namespace Poi {

  class Error {
  private:
    std::string message;

  public:
    explicit Error(
      const std::string &message // No trailing line break
    );
    explicit Error(
      const std::string &message, // No trailing line break
      const std::string &source_name,
      const std::string &source
    );
    explicit Error(
      const std::string &message, // No trailing line break
      const std::string &source_name,
      const std::string &source,
      size_t start_pos, // Inclusive
      size_t end_pos // Exclusive
    );
    virtual ~Error();
    std::string what() const; // No trailing line break
  };

}

#endif
