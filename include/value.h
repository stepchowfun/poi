/*
  This header declares the data types for values.
*/

#ifndef POI_VALUE_H
#define POI_VALUE_H

#include <cstddef>
#include <poi/string_pool.h>
#include <string>

namespace Poi {
  // A forward declaration for the member classes
  class Value;

  enum class ValueType {
    FIXPOINT,
    FUNCTION
  };

  const char * const ValueTypeName[] = {
    "FIXPOINT",
    "FUNCTION"
  };

  class FixpointMembers {
  public:
    Value *target;
  };

  class FunctionMembers {
  public:
    std::size_t body; // A pointer to the first bytecode of the body
    std::size_t frame_size; // The number of slots to allocate on top of the
                            // return address, including the captures and the
                            // argument
    std::size_t num_captures; // The number of captures
    Value **captures; // The capture list
  };

  class Value {
  public:
    ValueType type;
    union {
      FixpointMembers fixpoint_members;
      FunctionMembers function_members;
    };

    std::string show(StringPool &pool) const;
  };
}

#endif
