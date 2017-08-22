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
  std::shared_ptr<IrBlock> compile_ast_to_ir(std::shared_ptr<const Term> term);

  // Compile IR into BC.
  std::shared_ptr<BytecodeBlock> compile_ir_to_bc(const IrBlock &ir_block);
}

#endif
