/*
  This header declares the interface to the interpreter.
*/

#ifndef POI_INTERPRETER_H
#define POI_INTERPRETER_H

#include <poi/instruction.h>
#include <poi/string_pool.h>
#include <string>

namespace Poi {
  class Program {
  public:
    size_t num_instructions;
    Instruction *instructions;

    std::string show(StringPool &pool) const;
  };
}

#endif
