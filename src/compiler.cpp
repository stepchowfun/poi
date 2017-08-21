#include <memory>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <poi/compiler.h>
#include <poi/ir.h>
#include <vector>

void Poi::compile_to_ir(
  std::shared_ptr<const Term> term,
  std::vector<BasicBlock> &basic_blocks
) {
  BasicBlock starting_block;
  std::unordered_map<std::size_t, VariableInfo> environment;
  term->emit_ir(basic_blocks, starting_block, 0, false, environment);
  starting_block.getInstructions()->push_back(
    std::make_shared<IrExit>(0, std::static_pointer_cast<const Node>(term))
  );
  basic_blocks.push_back(starting_block);
}

void Poi::free_bytecode(std::vector<Poi::Bytecode> &program) {
  for (auto &bytecode : program) {
    bytecode.free();
  }
}
