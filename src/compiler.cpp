#include <memory>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <poi/compiler.h>
#include <poi/ir.h>
#include <vector>

std::shared_ptr<Poi::BasicBlock> Poi::compile_ast_to_ir(
  std::shared_ptr<const Poi::Term> term
) {
  auto block = std::make_shared<BasicBlock>();
  std::unordered_map<std::size_t, VariableInfo> environment;
  term->emit_ir(term, *block, 0, false, environment);
  block->get_instructions()->push_back(
    std::make_shared<IrExit>(0, std::static_pointer_cast<const Node>(term))
  );
  return block;
}

std::shared_ptr<std::vector<Poi::Bytecode>> Poi::compile_ir_to_bc(
  const Poi::BasicBlock &basic_block
) {
  std::vector<Bytecode> program;
  auto bytecode = std::make_shared<std::vector<Bytecode>>();
  basic_block.emit_bytecode(program, *bytecode);
  std::ptrdiff_t offset = bytecode->size();
  bytecode->insert(bytecode->end(), program.begin(), program.end());
  for (auto &bc : *bytecode) {
    bc.relocate(offset);
  }
  return bytecode;
}
