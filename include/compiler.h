/*
  This header declares the interface to the compiler.
*/

#ifndef POI_COMPILER_H
#define POI_COMPILER_H

#include <cstddef>
#include <memory>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <vector>

namespace Poi {
  void compile(
    const Poi::Term &term,
    std::vector<Poi::Bytecode> &program,
    std::size_t &start_program_counter, // Output parameter only
    std::size_t &start_stack_size // Output parameter only
  );

  // Delete all the memory pointed to by the bytecode. Note that the Bytecodes
  // themselves are owned by the vector and will be freed when its destructor
  // is called.
  void free(std::vector<Poi::Bytecode> &program);
}

#endif
