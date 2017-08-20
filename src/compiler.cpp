#include <memory>
#include <poi/ast.h>
#include <poi/bytecode.h>
#include <poi/compiler.h>
#include <vector>

std::size_t Poi::compile(
  const Poi::Term &term,
  std::vector<Poi::Bytecode> &program
) {
  std::vector<Poi::Bytecode> expression;
  std::unordered_map<std::size_t, std::size_t> environment;
  term.emit_bytecode(program, expression, environment, 0, true);
  std::size_t start = program.size();
  program.insert(program.end(), expression.begin(), expression.end());

  Bytecode return_bytecode;
  return_bytecode.type = Poi::BytecodeType::RETURN;
  return_bytecode.return_args.value = 0;
  program.push_back(return_bytecode);
  return start;
}

void Poi::free(std::vector<Poi::Bytecode> &program) {
  for (auto &bytecode : program) {
    bytecode.free();
  }
}
