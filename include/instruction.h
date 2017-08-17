/*
  This header declares a class for instructions.
*/

#ifndef POI_INSTRUCTION_H
#define POI_INSTRUCTION_H

#include <poi/ast.h>
#include <poi/string_pool.h>
#include <string>

namespace Poi {
  enum class InstructionType {
    COPY,
    CASE,
    CALL_NON_TAIL,
    CALL_TAIL,
    CREATE_PROXY,
    DATA,
    DATA_TYPE,
    FUNCTION,
    MEMBER,
    RETURN,
    UPDATE_PROXY
  };

  const char * const InstructionTypeName[] = {
    "COPY",
    "CASE",
    "CALL_NON_TAIL",
    "CALL_TAIL",
    "CREATE_PROXY",
    "DATA",
    "DATA_TYPE",
    "FUNCTION",
    "MEMBER",
    "RETURN",
    "UPDATE_PROXY"
  };

  class Instruction {
  public:
    const InstructionType type;
    const Node * const node;

    std::string show(StringPool &pool) const;
  };
}

#endif
