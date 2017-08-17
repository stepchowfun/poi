#include <poi/interpreter.h>

std::string Poi::Program::show(StringPool &pool) const {
  std::string result;
  for (size_t i = 0; i < num_instructions; ++i) {
    result += instructions[i].show(pool) + "\n";
  }
  return result;
}
