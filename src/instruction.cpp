#include <poi/instruction.h>
#include <type_traits>

std::string Poi::Instruction::show(Poi::StringPool &pool) const {
  std::string result = InstructionTypeName[
    static_cast<typename std::underlying_type<InstructionType>::type>(type)
  ];

  switch (type) {
    case Poi::InstructionType::BEGIN_FIXPOINT:
      result +=
        " " + std::to_string(begin_fixpoint_args.destination) +
        " " + std::to_string(begin_fixpoint_args.num_references);
      break;
    case Poi::InstructionType::CALL_NON_TAIL:
      result += " " + std::to_string(call_non_tail_args.function);
      break;
    case Poi::InstructionType::CALL_TAIL:
      result +=
        " " + std::to_string(call_tail_args.function) +
        " " + std::to_string(call_tail_args.frame_size);
      break;
    case Poi::InstructionType::COPY:
      result +=
        " " + std::to_string(copy_args.destination) +
        " " + std::to_string(copy_args.source);
      break;
    case Poi::InstructionType::CREATE_FUNCTION:
      result +=
        " " + std::to_string(create_function_args.destination) +
        " " + std::to_string(create_function_args.body) +
        " " + std::to_string(create_function_args.frame_size) +
        " [" + std::to_string(create_function_args.captures[0]);
      for (size_t i = 1; i < create_function_args.num_captures; i++) {
        result += ", " +
          std::to_string(create_function_args.captures[i]);
      }
      result += "]";
      break;
    case Poi::InstructionType::END_FIXPOINT:
      result +=
        " " + std::to_string(end_fixpoint_args.fixpoint) +
        " " + std::to_string(end_fixpoint_args.target);
      break;
    case Poi::InstructionType::RETURN:
      result += " " + std::to_string(return_args.frame_size);
      break;
    default:
      result = "`show` is not supported for " + std::string(
        InstructionTypeName[
          static_cast<typename std::underlying_type<InstructionType>::type>(
            type
          )
        ]
      ) + ".";
      break;
  }
  return result + "\n";
}
