/*
  This header declares the interface to the interpreter.
*/

#ifndef POI_INTERPRETER_H
#define POI_INTERPRETER_H

#include <cstddef>
#include <poi/bytecode.h>
#include <poi/value.h>
#include <vector>

namespace Poi {
  // Interpret a bytecode program.
  Value * interpret(
    Poi::Bytecode *program,
    std::size_t program_size,
    std::size_t start
  );
}

#endif
