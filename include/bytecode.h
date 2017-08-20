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
    END_FIXPOINT,
    CREATE_FUNCTION,
    RETURN
  };

  const char * const BytecodeTypeName[] = {
    "BEGIN_FIXPOINT",
    "CALL_NON_TAIL",
    "CALL_TAIL",
    "COPY",
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

  // Name: BEGIN_FIXPOINT
  //
  // Description:
  //   Create a fixpoint value. Fixpoint values are used as placeholders in
  //   recursive definitions. The END_FIXPOINT instruction "ties the knot" by
  //   updating all the recursive references to point to the value being
  //   defined.
  class BeginFixpointArguments {
  public:
    std::size_t destination; // (SP - destination) will contain the new
                             // fixpoint value.
  };

  // Name: CALL_NON_TAIL
  //
  // Description:
  //   Call a function. This instruction allocates a new stack frame with the
  //   captured values from the closure at the top. Then the following are
  //   pushed onto the stack: the argument, the return address, and the base
  //   pointer.
  class CallNonTailArguments {
  public:
    std::size_t destination; // (SP - destination) will contain the return
                             // value.
    std::size_t function; // (SP - function) contains the function to call.
    std::size_t argument; // (SP - arugment) contains the argument.
  };

  // Name: CALL_TAIL
  //
  // Description:
  //   Call a function. This instruction first pops the current frame off the
  //   stack. Then it allocates a new stack frame with the captured values
  //   from the closure at the top. Finally, the following are pushed onto the
  //   stack: the argument, the previous return address, and the previous base
  //   pointer. This allows the callee to reuse the memory from the caller's
  //   stack frame.
  class CallTailArguments {
  public:
    std::size_t destination; // (SP - destination) will contain the return
                             // value.
    std::size_t function; // (SP - function) contains the function to call.
    std::size_t argument; // (SP - arugment) contains the argument.
  };

  // Name: COPY
  //
  // Description:
  //   Copy the contents of (SP - source) into (SP - destination). This
  //   instruction does not make a copy of the value on the heap, it only
  //   copies the pointer on the stack.
  class CopyArguments {
  public:
    std::size_t destination; // (SP - destination) will contain the result.
    std::size_t source; // (SP - source) contains the pointer to copy.
  };

  // Name: CREATE_FUNCTION
  //
  // Description:
  //   Create a function.
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

  // Name: END_FIXPOINT
  //
  // Description:
  //   "Tie the knot" for a recursive definition by updating the fixpoint value
  //   to point to the recursive value that was just defined.
  class EndFixpointArguments {
  public:
    std::size_t fixpoint; // (SP - fixpoint) contains the fixpoint value.
    std::size_t target; // (SP - target) contains the recursive value.
  };

  // Name: RETURN
  //
  // Description:
  //   Return a value to the caller, pop the current frame off the stack, and
  //   jump to the return address.
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
      EndFixpointArguments end_fixpoint_args;
      ReturnArguments return_args;
    };

    std::string show(StringPool &pool) const;
  };
}

#endif
