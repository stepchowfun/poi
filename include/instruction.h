/*
  This header declares a class for instructions.
*/

#ifndef POI_INSTRUCTION_H
#define POI_INSTRUCTION_H

#include <poi/ast.h>
#include <poi/string_pool.h>
#include <string>

namespace Poi {
  enum class InstructionType {
    BEGIN_FIXPOINT,
    CALL_NON_TAIL,
    CALL_TAIL,
    COPY,
    END_FIXPOINT,
    CREATE_FUNCTION,
    RETURN
  };

  const char * const InstructionTypeName[] = {
    "BEGIN_FIXPOINT",
    "CALL_NON_TAIL",
    "CALL_TAIL",
    "COPY",
    "END_FIXPOINT",
    "CREATE_FUNCTION",
    "RETURN"
  };

  // Name: BEGIN_FIXPOINT
  //
  // Description:
  //   Create a fixpoint value. Fixpoint values are used as placeholders in
  //   recursive definitions. The END_FIXPOINT instruction "ties the knot" by
  //   updating all the recursive references to point to the value being
  //   defined.
  class BeginFixpointArguments {
  public:
    size_t destination; // (SP - destination) will contain the new fixpoint
                        // value.
    size_t num_references; // The number of functions that will capture this
                           // fixpoint value
  };

  // Name: CALL_NON_TAIL
  //
  // Description:
  //   Call a function. This instruction pushes a return address onto the stack
  //   so the caller can return back to the callee. It then allocates a new
  //   stack frame for the callee to use.
  class CallNonTailArguments {
  public:
    size_t destination; // (SP - destination) will contain the return value.
    size_t function; // (SP - function) contains the function to call.
    size_t argument; // (SP - arugment) contains the argument.
  };

  // Name: CALL_TAIL
  //
  // Description:
  //   Call a function. This instruction does not push a new return address
  //   onto the stack. Instead, the callee will return to the caller's caller.
  //   This function pops the current stack frame before allocating a new one
  //   for the callee, so the callee will reuse the stack space previously
  //   allocated for the caller.
  class CallTailArguments {
  public:
    size_t destination; // (SP - destination) will contain the return value.
    size_t function; // (SP - function) contains the function to call.
    size_t argument; // (SP - arugment) contains the argument.
    size_t frame_size; // (SP - frame_size) contains the instruction pointer.
  };

  // Name: COPY
  //
  // Description:
  //   Copy the contents of (SP - source) into (SP - destination). This
  //   instruction does not make a copy of the value on the heap, it only
  //   copies the pointer on the stack.
  class CopyArguments {
  public:
    size_t destination; // (SP - destination) will contain the result.
    size_t source; // (SP - source) contains the pointer to copy.
  };

  // Name: CREATE_FUNCTION
  //
  // Description:
  //   Create a function.
  class CreateFunctionArguments {
  public:
    size_t destination; // (SP - destination) will contain the new function.
    size_t body; // A pointer to the first instruction of the body
    size_t frame_size; // The number of slots to allocate on top of the return
                       // address, including the arguments
    size_t num_captures; // The number of free variables of the function
    size_t *captures; // An array of stack indices (counting down from SP)
                      // which refer to values to be captured
  };

  // Name: END_FIXPOINT
  //
  // Description:
  //   "Tie the knot" for a recursive definition by updating the fixpoint value
  //   to point to the recursive value that was just defined.
  class EndFixpointArguments {
  public:
    size_t fixpoint; // (SP - fixpoint) contains the fixpoint value.
    size_t target; // (SP - target) contains the recursive value.
  };

  // Name: RETURN
  //
  // Description:
  //   Return a value to the caller, pop the current frame off the stack, and
  //   jump to the return address.
  class ReturnArguments {
  public:
    size_t value; // (SP - value) contains the return value.
    size_t frame_size; // (SP - frame_size) contains the return address.
  };

  class Instruction {
  public:
    const Node *node;
    InstructionType type;
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
