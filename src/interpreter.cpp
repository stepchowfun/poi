#include <poi/error.h>
#include <poi/interpreter.h>
#include <poi/value.h>

size_t resize_pointer_stack(
  std::size_t * stack,
  std::size_t current_stack_size
) {
  // Return the new stack size.
  return 0;
}

size_t resize_value_stack(
  Poi::Value ** stack,
  std::size_t current_stack_size
) {
  // Return the new stack size.
  return 0;
}

Poi::Value * Poi::interpret(
  Poi::Bytecode * program,
  std::size_t program_size,
  std::size_t start
) {
  std::size_t value_stack_size = 0;
  Poi::Value ** value_stack = new Poi::Value*[value_stack_size];

  std::uintptr_t instruction_pointer = start;
  while (instruction_pointer < program_size) {
    auto bytecode = program[instruction_pointer];
    switch (bytecode.type) {
      case BytecodeType::BEGIN_FIXPOINT:
      {
        Value * fixpoint_value = new Value;
        fixpoint_value->type = ValueType::FIXPOINT;
        fixpoint_value->fixpoint_members.target = nullptr;
        value_stack[bytecode.begin_fixpoint_args.destination] =
          fixpoint_value;
        instruction_pointer++;
        break;
      }
      default:
        std::string bytecode_string = BytecodeTypeName[
          static_cast<typename std::underlying_type<
            BytecodeType>::type
          >(bytecode.type)
        ];
        throw Error(
          "interpret(...) is not implemented for '" + bytecode_string + "'"
        );
    }
  }

  return value_stack[0];
}
