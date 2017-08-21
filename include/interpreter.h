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
    Bytecode * program,
    std::size_t program_size, // Number of Bytecodes (not bytes)
    std::size_t start_program_counter,
    std::size_t start_stack_size // Number of stack locations (not bytes)
  );
}

#endif
