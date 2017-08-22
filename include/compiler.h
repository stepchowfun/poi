/*
  This header declares the interface to the compiler.
*/

#ifndef POI_COMPILER_H
#define POI_COMPILER_H

#include <cstddef>
#include <memory>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <poi/ir.h>
#include <vector>

namespace Poi {
  // Compile an AST into IR.
  std::shared_ptr<BasicBlock> compile_ast_to_ir(
    std::shared_ptr<const Term> term
  );

  // Compile IR into BC.
  std::shared_ptr<std::vector<Bytecode>> compile_ir_to_bc(
    const BasicBlock &basic_block
  );
}

#endif
