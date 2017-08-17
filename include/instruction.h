/*
  This header declares a class for instructions.
*/

#ifndef POI_INSTRUCTION_H
#define POI_INSTRUCTION_H

#include <poi/ast.h>

namespace Poi {
  enum class InstructionType {
    COPY
  };

  const char * const InstructionTypeName[] = {
    "COPY"
  };

  class Instruction {
  public:
    const InstructionType type;
    const Node * const node;
  };
}

#endif
