#include <poi/bytecode.h>
#include <poi/error.h>
#include <type_traits>

void Poi::Bytecode::relocate(std::ptrdiff_t offset) {
  std::string type_name = BytecodeTypeName[
    static_cast<typename std::underlying_type<BytecodeType>::type>(type)
  ];

  switch (type) {
    case BytecodeType::BEGIN_FIXPOINT: {
      break;
    }
    case BytecodeType::CALL: {
      break;
    }
    case BytecodeType::CREATE_FUNCTION: {
      create_function_args.body += offset;
      break;
    }
    case BytecodeType::DEREF_FIXPOINT: {
      break;
    }
    case BytecodeType::END_FIXPOINT: {
      break;
    }
    case BytecodeType::EXIT: {
      break;
    }
    case BytecodeType::MOVE: {
      break;
    }
    case BytecodeType::RETURN: {
      break;
    }
    case BytecodeType::TAIL_CALL: {
      break;
    }
    default: {
      throw Error("relocate(...) is not implemented for '" + type_name + "'.");
    }
  }
}

std::string Poi::Bytecode::show() const {
  std::string result = BytecodeTypeName[
    static_cast<typename std::underlying_type<BytecodeType>::type>(type)
  ];

  switch (type) {
    case BytecodeType::BEGIN_FIXPOINT: {
      result +=
        " destination=" + std::to_string(begin_fixpoint_args.destination);
      break;
    }
    case BytecodeType::CALL: {
      result +=
        " destination=" + std::to_string(call_non_tail_args.destination) +
        " function=" + std::to_string(call_non_tail_args.function) +
        " argument=" + std::to_string(call_non_tail_args.argument);
      break;
    }
    case BytecodeType::CREATE_FUNCTION: {
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
    case BytecodeType::DEREF_FIXPOINT: {
      result +=
        " destination=" + std::to_string(deref_fixpoint_args.destination) +
        " source=" + std::to_string(deref_fixpoint_args.fixpoint);
      break;
    }
    case BytecodeType::END_FIXPOINT: {
      result +=
        " fixpoint=" + std::to_string(end_fixpoint_args.fixpoint) +
        " target=" + std::to_string(end_fixpoint_args.target);
      break;
    }
    case BytecodeType::EXIT: {
      result +=
        " value=" + std::to_string(exit_args.value);
      break;
    }
    case BytecodeType::MOVE: {
      result +=
        " destination=" + std::to_string(move_args.destination) +
        " source=" + std::to_string(move_args.source);
      break;
    }
    case BytecodeType::RETURN: {
      result +=
        " value=" + std::to_string(return_args.value);
      break;
    }
    case BytecodeType::TAIL_CALL: {
      result +=
        " function=" + std::to_string(call_tail_args.function) +
        " argument=" + std::to_string(call_tail_args.argument);
      break;
    }
    default: {
      throw Error("show(...) is not implemented for '" + result + "'.");
    }
  }
  return result;
}

void Poi::BytecodeBlock::push(
  Poi::Bytecode &bc,
  std::shared_ptr<const Poi::Node> node
) {
  bytecode.push_back(bc);
  nodes.push_back(node);
}

void Poi::BytecodeBlock::append(Poi::BytecodeBlock &block) {
  bytecode.insert(
    bytecode.end(),
    block.bytecode.begin(),
    block.bytecode.end()
  );
  nodes.insert(
    nodes.end(),
    block.nodes.begin(),
    block.nodes.end()
  );
}

std::size_t Poi::BytecodeBlock::size() const {
  return bytecode.size();
}

std::string Poi::BytecodeBlock::show() const {
  std::string result;
  for (std::size_t i = 0; i < bytecode.size(); ++i) {
    result += std::to_string(i) + " " + bytecode[i].show() + "\n";
  }
  return result;
}
