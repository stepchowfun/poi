#include <poi/bytecode.h>
#include <poi/error.h>
#include <type_traits>

void Poi::Bytecode::free() {
  switch (type) {
    case Poi::BytecodeType::BEGIN_FIXPOINT: {
      break;
    }
    case Poi::BytecodeType::CALL_NON_TAIL: {
      break;
    }
    case Poi::BytecodeType::CALL_TAIL: {
      break;
    }
    case Poi::BytecodeType::COPY: {
      break;
    }
    case Poi::BytecodeType::CREATE_FUNCTION: {
      delete [] create_function_args.captures;
      break;
    }
    case Poi::BytecodeType::DEREF_FIXPOINT: {
      break;
    }
    case Poi::BytecodeType::END_FIXPOINT: {
      break;
    }
    case Poi::BytecodeType::RETURN: {
      break;
    }
    default: {
      std::string name = BytecodeTypeName[
        static_cast<typename std::underlying_type<BytecodeType>::type>(type)
      ];
      throw Error("free(...) is not implemented for '" + name + "'.");
    }
  }
}

std::string Poi::Bytecode::show() const {
  std::string result = BytecodeTypeName[
    static_cast<typename std::underlying_type<BytecodeType>::type>(type)
  ];

  switch (type) {
    case Poi::BytecodeType::BEGIN_FIXPOINT: {
      result +=
        " destination=" + std::to_string(begin_fixpoint_args.destination);
      break;
    }
    case Poi::BytecodeType::CALL_NON_TAIL: {
      result +=
        " destination=" + std::to_string(call_non_tail_args.destination) +
        " function=" + std::to_string(call_non_tail_args.function) +
        " argument=" + std::to_string(call_non_tail_args.argument);
      break;
    }
    case Poi::BytecodeType::CALL_TAIL: {
      result +=
        " function=" + std::to_string(call_tail_args.function) +
        " argument=" + std::to_string(call_tail_args.argument);
      break;
    }
    case Poi::BytecodeType::COPY: {
      result +=
        " destination=" + std::to_string(copy_args.destination) +
        " source=" + std::to_string(copy_args.source);
      break;
    }
    case Poi::BytecodeType::CREATE_FUNCTION: {
      result +=
        " destination=" + std::to_string(create_function_args.destination) +
        " body=" + std::to_string(create_function_args.body) +
        " frame_size=" + std::to_string(create_function_args.frame_size) +
        " captures=[";
      for (std::size_t i = 0; i < create_function_args.num_captures; i++) {
        if (i != 0) {
          result += ", ";
        }
        result += std::to_string(create_function_args.captures[i]);
      }
      result += "]";
      break;
    }
    case Poi::BytecodeType::DEREF_FIXPOINT: {
      result +=
        " destination=" + std::to_string(deref_fixpoint_args.destination) +
        " source=" + std::to_string(deref_fixpoint_args.source);
      break;
    }
    case Poi::BytecodeType::END_FIXPOINT: {
      result +=
        " fixpoint=" + std::to_string(end_fixpoint_args.fixpoint) +
        " target=" + std::to_string(end_fixpoint_args.target);
      break;
    }
    case Poi::BytecodeType::RETURN: {
      result +=
        " value=" + std::to_string(return_args.value);
      break;
    }
    default: {
      throw Error("show(...) is not implemented for '" + result + "'.");
    }
  }
  return result;
}
