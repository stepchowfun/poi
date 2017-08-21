#include <memory>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <poi/compiler.h>
#include <vector>

void Poi::compile(
  const Poi::Term &term,
  std::vector<Poi::Bytecode> &program,
  std::size_t &start_program_counter,
  std::size_t &start_stack_size
) {
  std::vector<Bytecode> expression;
  std::unordered_map<std::size_t, VariableInfo> environment;
  start_stack_size = term.emit_bytecode(
    program,
    expression,
    environment,
    0,
    false
  );
  start_program_counter = program.size();
  program.insert(program.end(), expression.begin(), expression.end());
  Bytecode exit_bytecode;
  exit_bytecode.type = Poi::BytecodeType::EXIT;
  exit_bytecode.return_args.value = 0;
  program.push_back(exit_bytecode);
}

void Poi::free(std::vector<Poi::Bytecode> &program) {
  for (auto &bytecode : program) {
    bytecode.free();
  }
}
