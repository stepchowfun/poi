/*
  This header declares a class for bytecode instructions.
*/

#ifndef POI_BYTECODE_H
#define POI_BYTECODE_H

#include <cstddef>
#include <poi/string_pool.h>
#include <string>

namespace Poi {
  enum class BytecodeType {
    BEGIN_FIXPOINT,
    CALL_NON_TAIL,
    CALL_TAIL,
    COPY,
    DEREF_FIXPOINT,
    END_FIXPOINT,
    CREATE_FUNCTION,
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
    "RETURN"
  };

  // Stack diagram:
  // - Base pointer (points to previous base pointer)
  // - Return address (points to the bytecode just after the CALL_NON_TAIL)
  // - Argument for the current function
  // - Capture 0
  // - ...
  // - Capture N
  // - ...
  // - Base pointer
  // - Return address
  // - ...

  class BeginFixpointArguments {
  public:
    std::size_t destination; // (SP - destination) will contain the new
                             // fixpoint value.
  };

  class CallNonTailArguments {
  public:
    std::size_t destination; // (SP - destination) will contain the return
                             // value.
    std::size_t function; // (SP - function) contains the function to call.
    std::size_t argument; // (SP - arugment) contains the argument.
  };

  class CallTailArguments {
  public:
    std::size_t destination; // (SP - destination) will contain the return
                             // value.
    std::size_t function; // (SP - function) contains the function to call.
    std::size_t argument; // (SP - arugment) contains the argument.
  };

  class CopyArguments {
  public:
    std::size_t destination; // (SP - destination) will contain the result.
    std::size_t source; // (SP - source) contains the pointer to copy.
  };

  class CreateFunctionArguments {
  public:
    std::size_t destination; // (SP - destination) will contain the new
                             // function.
    std::size_t body; // A pointer to the first bytecode of the body
    std::size_t frame_size; // The number of slots to allocate on top of the
                            // return address, including the captures and the
                            // argument
    std::size_t num_captures; // The number of free variables of the function
    std::size_t * captures; // An array of stack indices (counting down from
                            // SP) which refer to values to be captured
  };

  class DerefFixpointArguments {
  public:
    std::size_t destination; // (SP - destination) will contain the result.
    std::size_t source; // (SP - source) contains the fixpoint to dereference.
  };

  class EndFixpointArguments {
  public:
    std::size_t fixpoint; // (SP - fixpoint) contains the fixpoint value.
    std::size_t target; // (SP - target) contains the recursive value.
  };

  class ReturnArguments {
  public:
    std::size_t value; // (SP - value) contains the return value.
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
      ReturnArguments return_args;
    };

    void free();
    std::string show() const;
  };
}

#endif
