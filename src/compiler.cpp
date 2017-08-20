#include <memory>
#include <poi/ast.h>
#include <poi/compiler.h>
#include <poi/instruction.h>
#include <vector>

std::size_t Poi::compile(
  const Poi::Term &term,
  std::vector<Poi::Instruction> &program
) {
  std::vector<Poi::Instruction> expression;
  std::unordered_map<std::size_t, std::size_t> environment;
  term.emit_instructions(program, expression, environment, 0, true);
  std::size_t start = program.size();
  program.insert(program.end(), expression.begin(), expression.end());
  return start;
}
