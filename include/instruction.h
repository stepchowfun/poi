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
    CALL_NON_TAIL,
    CALL_TAIL,
    COPY,
    CREATE_PROXY,
    FUNCTION,
    RETURN,
    UPDATE_PROXY
  };

  const char * const InstructionTypeName[] = {
    "CALL_NON_TAIL",
    "CALL_TAIL",
    "COPY",
    "CREATE_PROXY",
    "FUNCTION",
    "RETURN",
    "UPDATE_PROXY"
  };

  // Name: CALL_NON_TAIL
  //
  // Description:
  //   Call a function. This instruction pushes a return address onto the stack
  //   so the caller can return back to the callee. It then allocates a new
  //   stack frame for the callee to use.  The callee is expected to overwrite
  //   the argument with the return value.
  //
  // Stack before (X is just a point of reference for comparison):
  //   X - Argument
  //     - ...
  //
  // Stack after jump to function body:
  //     - ...
  //     - Capture N
  //     - ...
  //     - Capture 0
  //     - Return address
  //   X - Argument
  //     - ...
  //
  // Stack after return:
  //   X - Return value
  //     - ...
  class CallNonTailArguments {
  public:
    size_t function; // (SP - function) contains to the function to call.
  };

  // Name: CALL_TAIL
  //
  // Description:
  //   Call a function. This instruction does not push a new return address
  //   onto the stack. Instead, the callee will return to the caller's caller.
  //   This function pops the current stack frame before allocating a new one
  //   for the callee, so the callee will reuse the stack space previously
  //   allocated for the caller.  The callee is expected to overwrite the
  //   caller's argument with its own return value.
  //
  // Stack before (X is just a point of reference for comparison):
  //     - ...
  //     - Return address
  //   X - Argument
  //     - ...
  //
  // Stack after jump to function body:
  //     - ...
  //     - Capture N
  //     - ...
  //     - Capture 0
  //     - Return address
  //   X - Argument
  //     - ...
  //
  // Stack after return:
  //   X - Return value
  //     - ...
  class CallTailArguments {
  public:
    size_t function; // (SP - function) contains the function to call.
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

  // Name: CREATE_PROXY
  //
  // Description:
  //   Create a proxy value that initially points to nothing. A proxy value is
  //   a value that acts as another value.
  class CreateProxyArguments {
  public:
    size_t destination; // (SP - destination) will contain the new proxy value.
  };

  // Name: FUNCTION
  //
  // Description:
  //   Create a function.
  class FunctionArguments {
  public:
    size_t destination; // (SP - destination) will contain the new function.
    size_t body; // A pointer to the first instruction of the body
    size_t frame_size; // The number of slots to allocate on top of the return
                       // address, including the arguments
    size_t num_captures; // The number of free variables of the function
    size_t *captures; // An array of stack indices (counting down from SP)
                      // which refer to values to be captured
  };

  // Name: RETURN
  //
  // Description:
  //   Pop (frame_size) slots off the stack, jump to the return address (which
  //   should be at the top of the stack at that point), and then pop the
  //   return address off the stack.
  //
  // Stack before (X is just a point of reference for comparison):
  //     - ...
  //     - Return address
  //   X - ...
  //
  // Stack after:
  //   X - ...
  class ReturnArguments {
  public:
    size_t frame_size; // (SP - frame_size) contains the return address.
  };

  // Name: UPDATE_PROXY
  //
  // Description:
  //   Set the value pointed to by a proxy value (in-place).
  class UpdateProxyArguments {
  public:
    size_t proxy; // (SP - proxy) contains the proxy value.
    size_t target; // (SP - target) contains the new target.
  };

  class Instruction {
  public:
    Node *node;
    InstructionType type;
    union {
      CopyArguments copy_args;
      CallNonTailArguments call_non_tail_args;
      CallTailArguments call_tail_args;
      CreateProxyArguments create_proxy_args;
      FunctionArguments function_args;
      ReturnArguments return_args;
      UpdateProxyArguments update_proxy_args;
    };

    std::string show(StringPool &pool) const;
  };
}

#endif
