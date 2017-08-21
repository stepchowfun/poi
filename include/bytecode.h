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
    "CREATE_FUNCTION",
    "DEREF_FIXPOINT",
    "END_FIXPOINT",
    "EXIT",
    "RETURN"
  };

  class BeginFixpointArguments {
  public:
    std::uint16_t destination;
  };

  class CallNonTailArguments {
  public:
    std::uint16_t destination;
    std::uint16_t function;
    std::uint16_t argument;
  };

  class CallTailArguments {
  public:
    std::uint16_t function;
    std::uint16_t argument;
  };

  class CopyArguments {
  public:
    std::uint16_t destination;
    std::uint16_t source;
  };

  class CreateFunctionArguments {
  public:
    std::uint16_t destination;
    std::uint16_t frame_size;
    std::uint16_t num_captures;
    std::uint16_t * captures;
    std::size_t body;
  };

  class DerefFixpointArguments {
  public:
    std::uint16_t destination;
    std::uint16_t fixpoint;
  };

  class EndFixpointArguments {
  public:
    std::uint16_t fixpoint;
    std::uint16_t target;
  };

  class ExitArguments {
  public:
    std::uint16_t value;
  };

  class ReturnArguments {
  public:
    std::uint16_t value;
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

    void relocate(std::ptrdiff_t offset); // Shift instruction pointers
    std::string show() const;
  };

  static_assert(
    std::is_pod<Bytecode>::value,
    "Poi::Bytecode must be a POD type."
  );
}

#endif
