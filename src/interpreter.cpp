#include <poi/error.h>
#include <poi/interpreter.h>
#include <poi/value.h>

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
      case BytecodeType::BEGIN_FIXPOINT: {
        Value * fixpoint_value = new Value;
        fixpoint_value->type = ValueType::FIXPOINT;
        fixpoint_value->fixpoint_members.target = nullptr;
        value_stack[bytecode.begin_fixpoint_args.destination] =
          fixpoint_value;
        break;
      }
      default: {
        throw Error(
          "interpret(...) is not implemented for '" + bytecode.show() + "'."
        );
      }
      instruction_pointer++;
    }
  }

  return value_stack[0];
}
