/*
  This header declares the interface to the interpreter.
*/

#ifndef POI_INTERPRETER_H
#define POI_INTERPRETER_H

#include <cstddef>
#include <poi/bytecode.h>
#include <poi/string_pool.h>
#include <poi/value.h>
#include <vector>

namespace Poi {
  // Interpret a BC program.
  Value * interpret(
    std::size_t start_stack_size, // Stack locations (not bytes)
    const BytecodeBlock &block,
    const Poi::StringPool &pool
  );
}

#endif
