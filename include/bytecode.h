/*
  This header declares a class for bytecode instructions.
*/

#ifndef POI_BYTECODE_H
#define POI_BYTECODE_H

#include <cstddef>
#include <cstdint>
#include <poi/string_pool.h>
#include <string>
#include <type_traits>

namespace Poi {
  enum class BytecodeType {
    BEGIN_FIXPOINT,
    CALL_NON_TAIL,
    CALL_TAIL,
    COPY,
    CREATE_FUNCTION,
    DEREF_FIXPOINT,
    END_FIXPOINT,
    EXIT,
    RETURN
  };

  const char * const BytecodeTypeName[] = {
    "BEGIN_FIXPOINT",
    "CALL_NON_TAIL",
    "CALL_TAIL",
    "COPY",
    "DEREF_FIXPOINT",
    "END_FIXPOINT",
    "CREATE_FUNCTION",
    "EXIT",
    "RETURN"
  };

  class BeginFixpointArguments {
  public:
    std::uint16_t destination; // (SP - destination) will contain the new
                               // fixpoint value.
  };

  class CallNonTailArguments {
  public:
    std::uint16_t destination; // (SP - destination) will contain the return
                               // value.
    std::uint16_t function; // (SP - function) contains the function to call.
    std::uint16_t argument; // (SP - arugment) contains the argument.
  };

  class CallTailArguments {
  public:
    std::uint16_t function; // (SP - function) contains the function to call.
    std::uint16_t argument; // (SP - arugment) contains the argument.
  };

  class CopyArguments {
  public:
    std::uint16_t destination; // (SP - destination) will contain the result.
    std::uint16_t source; // (SP - source) is the slot to copy.
  };

  class CreateFunctionArguments {
  public:
    std::uint16_t destination; // (SP - destination) will contain the new
                               // function.
    std::uint16_t frame_size; // The number of slots to allocate on the stack
    std::uint16_t num_captures; // The number of free variables of the function
    std::uint16_t * captures; // An array of stack indices (counting down from
                              // SP) which refer to values to be captured
    std::size_t body; // The index of the first bytecode of the body
  };

  class DerefFixpointArguments {
  public:
    std::uint16_t destination; // (SP - destination) will contain the result.
    std::uint16_t fixpoint; // (SP - source) contains the fixpoint to
                            // dereference.
  };

  class EndFixpointArguments {
  public:
    std::uint16_t fixpoint; // (SP - fixpoint) contains the fixpoint value.
    std::uint16_t target; // (SP - target) contains the recursive value.
  };

  class ExitArguments {
  public:
    std::uint16_t value; // (SP - value) contains the exit value.
  };

  class ReturnArguments {
  public:
    std::uint16_t value; // (SP - value) contains the return value.
  };

  class Bytecode {
  public:
    BytecodeType type;

    union {
      BeginFixpointArguments begin_fixpoint_args;
      CallNonTailArguments call_non_tail_args;
      CallTailArguments call_tail_args;
      CopyArguments copy_args;
      CreateFunctionArguments create_function_args;
      DerefFixpointArguments deref_fixpoint_args;
      EndFixpointArguments end_fixpoint_args;
      ExitArguments exit_args;
      ReturnArguments return_args;
    };

    void free();
    std::string show() const;
  };

  static_assert(
    std::is_pod<Bytecode>::value,
    "Poi::Bytecode must be a POD type."
  );
}

#endif
