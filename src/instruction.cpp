#include <poi/instruction.h>
#include <type_traits>

std::string Poi::Instruction::show(Poi::StringPool &pool) const {
  return std::string(
    InstructionTypeName[
      static_cast<typename std::underlying_type<InstructionType>::type>(type)
    ]
  );
}
