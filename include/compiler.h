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
  // Compile a Term into IR.
  void compile_to_ir(
    std::shared_ptr<const Term> term,
    std::vector<BasicBlock> &basic_blocks // The last block will be the start
  );

  // Delete all the memory pointed to by the bytecode program. Note that the
  // Bytecodes themselves are owned by the vector and will be freed when the
  // vector's destructor is called.
  void free_bytecode(std::vector<Bytecode> &program);
}

#endif
