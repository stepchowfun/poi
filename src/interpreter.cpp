#include <poi/error.h>
#include <poi/interpreter.h>
#include <poi/value.h>

namespace Poi {
  class Frame {
    public:
      std::size_t base_pointer;
      std::size_t return_address;
  };
}

Poi::Value * Poi::interpret(
  Poi::Bytecode * program,
  std::size_t program_size,
  std::size_t start
) {
  std::size_t value_stack_buffer_size = 524288;
  Poi::Value ** value_stack = new Poi::Value*[value_stack_buffer_size];

  std::uintptr_t instruction_pointer = start;
  while (instruction_pointer < program_size) {
    auto bytecode = program[instruction_pointer];
    switch (bytecode.type) {
      case BytecodeType::BEGIN_FIXPOINT: {
        Value * fixpoint_value = new Value;
        fixpoint_value->type = ValueType::FIXPOINT;
        fixpoint_value->fixpoint_members.target = nullptr;
        value_stack[bytecode.begin_fixpoint_args.destination] = fixpoint_value;
        break;
      }
      case BytecodeType::CALL_NON_TAIL: {
        break;
      }
      case BytecodeType::CALL_TAIL: {
        break;
      }
      case BytecodeType::COPY: {
        break;
      }
      case BytecodeType::CREATE_FUNCTION: {
        break;
      }
      case BytecodeType::DEREF_FIXPOINT: {
        break;
      }
      case BytecodeType::END_FIXPOINT: {
        break;
      }
      case BytecodeType::RETURN: {
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
