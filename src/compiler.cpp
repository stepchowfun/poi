#include <memory>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <poi/compiler.h>
#include <poi/ir.h>
#include <vector>

std::shared_ptr<Poi::BasicBlock> Poi::compile_to_ir(
  std::shared_ptr<const Poi::Term> term
) {
  auto block = std::make_shared<BasicBlock>();
  std::unordered_map<std::size_t, VariableInfo> environment;
  term->emit_ir(*block, 0, false, environment);
  block->get_instructions()->push_back(
    std::make_shared<IrExit>(0, std::static_pointer_cast<const Node>(term))
  );
  return block;
}

void Poi::free_bytecode(std::vector<Poi::Bytecode> &program) {
  for (auto &bytecode : program) {
    bytecode.free();
  }
}
