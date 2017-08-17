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

  class CallNonTailArguments {
  public:
    size_t fun;
  };

  class CallTailArguments {
  public:
    size_t fun;
    size_t frame_size;
  };

  class CopyArguments {
  public:
    size_t destination;
    size_t source;
  };

  class CreateProxyArguments {
  public:
    size_t destination;
  };

  class FunctionArguments {
  public:
    size_t destination;
    size_t body;
    size_t frame_size;
    size_t num_captures;
    size_t *captures;
  };

  class ReturnArguments {
  public:
    size_t frame_size;
  };

  class UpdateProxyArguments {
  public:
    size_t proxy;
    size_t target;
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
