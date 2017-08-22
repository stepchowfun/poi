#include <cassert>
#include <cstring>
#include <poi/error.h>
#include <poi/interpreter.h>
#include <poi/value.h>

#ifndef NDEBUG
  #include <iostream>
#endif

namespace Poi {
  const std::size_t stack_allocation_factor = 16;
  const std::size_t min_stack_buffer_size = 1024;

  class Frame {
    public:
      std::size_t base_pointer;
      std::size_t return_address;
  };
}

template <typename T> void resize_buffer_if_needed(
  std::size_t requested_logical_size,
  std::size_t &current_logical_size,
  std::size_t &current_physical_size,
  T * &buffer
  #ifndef NDEBUG
    , std::string buffer_name
  #endif
) {
  std::size_t new_physical_size = (current_physical_size == 0)
    ? 1
    : current_physical_size;
  while (requested_logical_size > new_physical_size) {
    new_physical_size *= Poi::stack_allocation_factor;
  }
  while (
    (
      requested_logical_size *
      Poi::stack_allocation_factor *
      Poi::stack_allocation_factor
    ) <= new_physical_size && new_physical_size >=
      Poi::min_stack_buffer_size * Poi::stack_allocation_factor
  ) {
    new_physical_size /= Poi::stack_allocation_factor;
  }
  if (current_physical_size != new_physical_size) {
    #ifndef NDEBUG
      std::cout
        << "REALLOCATING "
        << buffer_name
        << ": "
        << current_physical_size
        << " -> "
        << new_physical_size
        << "\n\n";
    #endif
    auto new_buffer = new T[new_physical_size];
    #ifndef NDEBUG
      memset(new_buffer, 0, new_physical_size * sizeof(T));
    #endif
    memcpy(new_buffer, buffer, sizeof(T) * current_logical_size);
    delete [] buffer;
    buffer = new_buffer;
    current_physical_size = new_physical_size;
  }
  current_logical_size = requested_logical_size;
}

Poi::Value * Poi::interpret(
  Poi::Bytecode * program,
  std::size_t start_stack_size
) {
  std::size_t value_stack_size = start_stack_size;
  std::size_t value_stack_buffer_size = Poi::min_stack_buffer_size;
  auto value_stack = new Value * [value_stack_buffer_size];
  #ifndef NDEBUG
    memset(value_stack, 0, value_stack_buffer_size * sizeof(Value *));
  #endif
  resize_buffer_if_needed(
    start_stack_size,
    value_stack_size,
    value_stack_buffer_size,
    value_stack
    #ifndef NDEBUG
      , "VALUE_STACK"
    #endif
  );

  std::size_t call_stack_size = 0;
  std::size_t call_stack_buffer_size = Poi::min_stack_buffer_size;
  auto call_stack = new Frame[call_stack_buffer_size];

  std::size_t program_counter = 0;
  while (true) {
    auto bytecode = program[program_counter];

    #ifndef NDEBUG
      std::cout << "CALL STACK:\n";
      if (call_stack_size > 0) {
        for (std::size_t i = call_stack_size - 1; true; --i) {
          std::cout
            << "  "
            << call_stack_size - 1 - i
            << " base_pointer="
            << call_stack[i].base_pointer
            << " return_address="
            << call_stack[i].return_address
            << "\n";
          if (i == 0) {
            break;
          }
        }
      }
      std::cout << "\nVALUE STACK:\n";
      if (value_stack_size > 0) {
        for (std::size_t i = value_stack_size - 1; true; --i) {
          if (value_stack[i]) {
            std::cout
              << "  "
              << value_stack_size - 1 - i
              << " "
              << value_stack[i]->show(0)
              << "\n";
          } else {
            std::cout
              << "  "
              << value_stack_size - 1 - i
              << " NULL\n";
          }
          if (i == 0) {
            break;
          }
        }
      }
      std::cout
        << "\nEXECUTING: "
        << program_counter
        << " "
        << bytecode.show()
        << "\n\n";
    #endif

    switch (bytecode.type) {
      case BytecodeType::BEGIN_FIXPOINT: {
        Value * fixpoint_value = new Value;
        fixpoint_value->type = ValueType::FIXPOINT;
        fixpoint_value->fixpoint_members.target = nullptr;
        value_stack[
          value_stack_size - 1 - bytecode.begin_fixpoint_args.destination
        ] = fixpoint_value;
        break;
      }
      case BytecodeType::CALL_NON_TAIL: {
        assert(
          value_stack[
            value_stack_size - 1 - bytecode.call_non_tail_args.function
          ]->type == ValueType::FUNCTION
        );
        auto &function_members = value_stack[
          value_stack_size - 1 - bytecode.call_non_tail_args.function
        ]->function_members;
        auto argument = value_stack[
          value_stack_size - 1 - bytecode.call_non_tail_args.argument
        ];
        auto base_pointer = value_stack_size;
        std::size_t new_value_stack_size =
          value_stack_size + function_members.frame_size;
        resize_buffer_if_needed(
          new_value_stack_size,
          value_stack_size,
          value_stack_buffer_size,
          value_stack
          #ifndef NDEBUG
            , "VALUE_STACK"
          #endif
        );
        value_stack[value_stack_size - 1] = argument;
        for (std::uint16_t i = 0; i < function_members.num_captures; ++i) {
          value_stack[value_stack_size - 2 - i] = function_members.captures[i];
        }

        std::size_t new_call_stack_size = call_stack_size + 1;
        resize_buffer_if_needed(
          new_call_stack_size,
          call_stack_size,
          call_stack_buffer_size,
          call_stack
          #ifndef NDEBUG
            , "CALL_STACK"
          #endif
        );
        call_stack[call_stack_size - 1].base_pointer = base_pointer;
        call_stack[call_stack_size - 1].return_address =
          program_counter + 1;

        program_counter = function_members.body;
        continue;
      }
      case BytecodeType::CALL_TAIL: {
        assert(
          value_stack[
            value_stack_size - 1 - bytecode.call_tail_args.function
          ]->type == ValueType::FUNCTION
        );
        auto &function_members = value_stack[
          value_stack_size - 1 - bytecode.call_tail_args.function
        ]->function_members;
        auto argument = value_stack[
          value_stack_size - 1 - bytecode.call_tail_args.argument
        ];
        std::size_t new_value_stack_size =
          call_stack[call_stack_size - 1].base_pointer +
          function_members.frame_size;
        resize_buffer_if_needed(
          new_value_stack_size,
          value_stack_size,
          value_stack_buffer_size,
          value_stack
          #ifndef NDEBUG
            , "VALUE_STACK"
          #endif
        );
        value_stack[value_stack_size - 1] = argument;
        for (std::uint16_t i = 0; i < function_members.num_captures; ++i) {
          value_stack[value_stack_size - 2 - i] = function_members.captures[i];
        }

        program_counter = function_members.body;
        continue;
      }
      case BytecodeType::COPY: {
        value_stack[value_stack_size - 1 - bytecode.copy_args.destination] =
          value_stack[value_stack_size - 1 - bytecode.copy_args.source];
        break;
      }
      case BytecodeType::CREATE_FUNCTION: {
        Value * function_value = new Value;
        function_value->type = ValueType::FUNCTION;
        function_value->function_members.body =
          bytecode.create_function_args.body;
        function_value->function_members.frame_size =
          bytecode.create_function_args.frame_size;
        function_value->function_members.num_captures =
          bytecode.create_function_args.num_captures;
        function_value->function_members.captures = new Value * [
          bytecode.create_function_args.num_captures
        ];
        for (
          std::uint16_t i = 0;
          i < bytecode.create_function_args.num_captures;
          ++i
        ) {
          function_value->function_members.captures[i] = value_stack[
            value_stack_size - 1 - bytecode.create_function_args.captures[i]
          ];
        }
        value_stack[
          value_stack_size - 1 - bytecode.create_function_args.destination
        ] = function_value;
        break;
      }
      case BytecodeType::DEREF_FIXPOINT: {
        assert(
          value_stack[
            value_stack_size - 1 - bytecode.deref_fixpoint_args.fixpoint
          ]->type == ValueType::FIXPOINT
        );
        auto target = value_stack[
          value_stack_size - 1 - bytecode.deref_fixpoint_args.fixpoint
        ]->fixpoint_members.target;
        assert(target != nullptr);
        value_stack[
          value_stack_size - 1 - bytecode.deref_fixpoint_args.destination
        ] = target;
        break;
      }
      case BytecodeType::END_FIXPOINT: {
        assert(
          value_stack[
            value_stack_size - 1 - bytecode.end_fixpoint_args.fixpoint
          ]->type == ValueType::FIXPOINT
        );
        value_stack[
          value_stack_size - 1 - bytecode.end_fixpoint_args.fixpoint
        ]->fixpoint_members.target = value_stack[
          value_stack_size - 1 - bytecode.end_fixpoint_args.target
        ];
        break;
      }
      case BytecodeType::EXIT: {
        auto result = value_stack[
          value_stack_size - 1 - bytecode.exit_args.value
        ];
        delete [] value_stack;
        delete [] call_stack;
        return result;
      }
      case BytecodeType::RETURN: {
        auto return_value = value_stack[
          value_stack_size - 1 - bytecode.return_args.value
        ];
        std::size_t new_value_stack_size = call_stack[
          call_stack_size - 1
        ].base_pointer;
        resize_buffer_if_needed(
          new_value_stack_size,
          value_stack_size,
          value_stack_buffer_size,
          value_stack
          #ifndef NDEBUG
            , "VALUE_STACK"
          #endif
        );
        program_counter = call_stack[call_stack_size - 1].return_address;
        std::size_t new_call_stack_size = call_stack_size - 1;
        resize_buffer_if_needed(
          new_call_stack_size,
          call_stack_size,
          call_stack_buffer_size,
          call_stack
          #ifndef NDEBUG
            , "CALL_STACK"
          #endif
        );
        assert(
          program[program_counter - 1].type == BytecodeType::CALL_NON_TAIL
        );
        value_stack[
          value_stack_size - 1 -
            program[program_counter - 1].call_non_tail_args.destination
        ] = return_value;
        continue;
      }
      default: {
        throw Error(
          "interpret(...) is not implemented for '" + bytecode.show() + "'."
        );
      }
    }
    program_counter++;
  }

  assert(false);
  return nullptr;
}
