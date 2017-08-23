#include <memory>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <poi/compiler.h>
#include <poi/ir.h>
#include <vector>

std::shared_ptr<Poi::IrBlock> Poi::compile_ast_to_ir(
  std::shared_ptr<const Poi::Term> term
) {
  auto block = std::make_shared<IrBlock>();
  std::unordered_map<std::size_t, VariableInfo> environment;
  term->emit_ir(term, *block, 0, false, environment);
  block->get_instructions()->push_back(
    std::make_shared<IrExit>(0, std::static_pointer_cast<const Node>(term))
  );
  return block;
}

std::shared_ptr<Poi::BytecodeBlock> Poi::compile_ir_to_bc(
  const Poi::IrBlock &ir_block
) {
  BytecodeBlock archive;
  auto current = std::make_shared<BytecodeBlock>();
  ir_block.emit_bytecode(archive, *current);
  std::ptrdiff_t offset = current->size();
  current->append(archive);
  for (auto &bc : current->bytecode) {
    bc.relocate(offset);
  }
  return current;
}
