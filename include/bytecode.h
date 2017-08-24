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
  using Register = std::uint16_t;

  enum class BytecodeType {
    BEGIN_FIXPOINT,
    CALL,
    CREATE_FUNCTION,
    DATA,
    DATA_TYPE,
    DEREF_FIXPOINT,
    END_FIXPOINT,
    EXIT,
    MEMBER,
    RETURN,
    TAIL_CALL
  };

  const char * const BytecodeTypeName[] = {
    "BEGIN_FIXPOINT",
    "CALL",
    "CREATE_FUNCTION",
    "DATA",
    "DATA_TYPE",
    "DEREF_FIXPOINT",
    "END_FIXPOINT",
    "EXIT",
    "MEMBER",
    "RETURN",
    "TAIL_CALL"
  };

  class BeginFixpointArgs {
  public:
    Register destination;
  };

  class CallArgs {
  public:
    Register destination;
    Register function;
    Register argument;
  };

  class CreateFunctionArgs {
  public:
    Register destination;
    Register frame_size;
    Register num_captures;
    Register * captures;
    std::size_t body;
  };

  class DataArgs {
  public:
    Register destination;
    Register * captures;
    std::size_t * parameters;
  };

  class DataTypeArgs {
  public:
    Register destination;
    Register * constructors;
  };

  class DerefFixpointArgs {
  public:
    Register destination;
    Register fixpoint;
  };

  class EndFixpointArgs {
  public:
    Register fixpoint;
    Register target;
  };

  class ExitArgs {
  public:
    Register value;
  };

  class MemberArgs {
  public:
    Register destination;
    Register object;
    std::size_t field;
  };

  class ReturnArgs {
  public:
    Register value;
  };

  class TailCallArgs {
  public:
    Register function;
    Register argument;
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
