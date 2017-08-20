/*
  This header declares the interface to the interpreter.
*/

#ifndef POI_INTERPRETER_H
#define POI_INTERPRETER_H

#include <poi/instruction.h>
#include <poi/value.h>
#include <vector>

namespace Poi {
  Value * interpret(
    Poi::Instruction *program,
    size_t program_size,
    size_t start
  );
}

#endif
