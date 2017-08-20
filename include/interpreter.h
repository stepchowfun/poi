/*
  This header declares the interface to the interpreter.
*/

#ifndef POI_INTERPRETER_H
#define POI_INTERPRETER_H

#include <poi/instruction.h>
#include <poi/value.h>
#include <vector>

namespace Poi {
  // Interpret a bytecode program.
  Value * interpret(
    Poi::Instruction *program,
    std::size_t program_size,
    std::size_t start
  );
}

#endif
