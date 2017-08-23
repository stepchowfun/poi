/*
  This header declares a class for bytecode instructions.
*/

#ifndef POI_BYTECODE_H
#define POI_BYTECODE_H

#include <cstddef>
#include <cstdint>
#include <poi/ast.h>
#include <poi/string_pool.h>
#include <string>
#include <type_traits>

namespace Poi {
  enum class BytecodeType {
    BEGIN_FIXPOINT,
    CALL,
    CREATE_FUNCTION,
    DEREF_FIXPOINT,
    END_FIXPOINT,
    EXIT,
    MOVE,
    RETURN,
    TAIL_CALL
  };

  const char * const BytecodeTypeName[] = {
    "BEGIN_FIXPOINT",
    "CALL",
    "CREATE_FUNCTION",
    "DEREF_FIXPOINT",
    "END_FIXPOINT",
    "EXIT",
    "MOVE",
    "RETURN",
    "TAIL_CALL"
  };

  class BeginFixpointArgs {
  public:
    std::uint16_t destination;
  };

  class CallArgs {
  public:
    std::uint16_t destination;
    std::uint16_t function;
    std::uint16_t argument;
  };

  class CreateFunctionArgs {
  public:
    std::uint16_t destination;
    std::uint16_t frame_size;
    std::uint16_t num_captures;
    std::uint16_t * captures;
    std::size_t body;
  };

  class DerefFixpointArgs {
  public:
    std::uint16_t destination;
    std::uint16_t fixpoint;
  };

  class EndFixpointArgs {
  public:
    std::uint16_t fixpoint;
    std::uint16_t target;
  };

  class ExitArgs {
  public:
    std::uint16_t value;
  };

  class MoveArgs {
  public:
    std::uint16_t destination;
    std::uint16_t source;
  };

  class ReturnArgs {
  public:
    std::uint16_t value;
  };

  class TailCallArgs {
  public:
    std::uint16_t function;
    std::uint16_t argument;
  };

  class Bytecode {
  public:
    BytecodeType type;
    union {
      BeginFixpointArgs begin_fixpoint_args;
      CallArgs call_non_tail_args;
      CreateFunctionArgs create_function_args;
      DerefFixpointArgs deref_fixpoint_args;
      EndFixpointArgs end_fixpoint_args;
      ExitArgs exit_args;
      MoveArgs move_args;
      ReturnArgs return_args;
      TailCallArgs call_tail_args;
    };

    void relocate(std::ptrdiff_t offset); // Shift instruction pointers
    std::string show() const;
  };

  static_assert(
    std::is_pod<Bytecode>::value,
    "Poi::Bytecode must be a POD type."
  );

  class BytecodeBlock {
  public:
    std::vector<Bytecode> bytecode;
    std::vector<std::shared_ptr<const Node>> nodes;

    void push(Bytecode &bc, std::shared_ptr<const Node> node);
    void append(BytecodeBlock &block);
    std::size_t size() const;
    std::string show() const;
  };
}

#endif
