#include <poi/ast.h>
#include <poi/error.h>
#include <poi/value.h>

///////////////////////////////////////////////////////////////////////////////
// Value                                                                     //
///////////////////////////////////////////////////////////////////////////////

Poi::Value::~Value() {
}

///////////////////////////////////////////////////////////////////////////////
// FunctionValue                                                             //
///////////////////////////////////////////////////////////////////////////////

Poi::FunctionValue::FunctionValue(
  std::shared_ptr<Poi::Abstraction> abstraction,
  std::shared_ptr<
    std::unordered_map<size_t, std::shared_ptr<Poi::Value>>
  > captures
) : abstraction(abstraction), captures(captures) {
}

std::string Poi::FunctionValue::show(const Poi::StringPool &pool) const {
  return abstraction->show(pool);
}

///////////////////////////////////////////////////////////////////////////////
// DataTypeValue                                                             //
///////////////////////////////////////////////////////////////////////////////

Poi::DataTypeValue::DataTypeValue(
  std::shared_ptr<Poi::DataType> data_type
) : data_type(data_type) {
}

std::string Poi::DataTypeValue::show(const Poi::StringPool &pool) const {
  return data_type->show(pool);
}

///////////////////////////////////////////////////////////////////////////////
// DataValue                                                                 //
///////////////////////////////////////////////////////////////////////////////

Poi::DataValue::DataValue(
  std::shared_ptr<Poi::DataTypeValue> type,
  std::size_t constructor,
  std::shared_ptr<
    std::unordered_map<size_t, std::shared_ptr<Poi::Value>>
  > captures
) : type(type), constructor(constructor), captures(captures) {
}

std::string Poi::DataValue::show(const Poi::StringPool &pool) const {
  std::string result = "(" + pool.find(constructor);
  for (auto &capture : *captures) {
    result += " " + capture.second->show(pool);
  }
  result += ")";
  return result;
}
