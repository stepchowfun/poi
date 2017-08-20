#include <poi/error.h>
#include <poi/value.h>
#include <type_traits>

std::string Poi::Value::show() const {
  std::string result = ValueTypeName[
    static_cast<typename std::underlying_type<ValueType>::type>(type)
  ];

  switch (type) {
    case Poi::ValueType::FIXPOINT: {
      if (fixpoint_members.target) {
        result += " target=" + fixpoint_members.target->show();
      } else {
        result += " target=null";
      }
      break;
    }
    case Poi::ValueType::FUNCTION: {
      result +=
        " body=" + std::to_string(function_members.body) +
        " frame_size=" + std::to_string(function_members.frame_size) +
        " captures=[";
      for (std::size_t i = 0; i < function_members.num_captures; i++) {
        if (i != 0) {
          result += ", ";
        }
        result += function_members.captures[i]->show();
      }
      result += "]";
      break;
    }
    default: {
      throw Error("show(...) is not implemented for '" + result + "'");
    }
  }

  return result;
}
